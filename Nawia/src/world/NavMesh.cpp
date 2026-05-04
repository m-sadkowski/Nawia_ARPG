#include "NavMesh.h"
#include <Logger.h>
#include <raymath.h>

#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>

#include <cstring>
#include <cmath>

namespace Nawia::World {

	namespace {

		constexpr float k_max_path_end_distance_sq = 1.5f;

	}

	NavMesh::NavMesh() : _navMesh(nullptr), _navQuery(nullptr) {}

	NavMesh::~NavMesh() {
		cleanup();
	}

	void NavMesh::cleanup() {
		dtFreeNavMesh(_navMesh);
		_navMesh = nullptr;
		dtFreeNavMeshQuery(_navQuery);
		_navQuery = nullptr;
	}

	bool NavMesh::buildFromModel(const Model& model, float scale) {
		cleanup();

		if (model.meshCount == 0) {
			Core::Logger::errorLog("NavMesh: Cannot build from an empty model.");
			return false;
		}

		Core::Logger::debugLog("NavMesh: Building from model with " + std::to_string(model.meshCount) + " meshes.");

		// 1. Gather all vertices and indices
		std::vector<float> verts;
		std::vector<int> tris;

		for (int m = 0; m < model.meshCount; ++m) {
			const Mesh& mesh = model.meshes[m];
			
			int vertexOffset = verts.size() / 3;

			// Add vertices
			for (int v = 0; v < mesh.vertexCount; ++v) {
				Vector3 pos = {
					mesh.vertices[v * 3 + 0],
					mesh.vertices[v * 3 + 1],
					mesh.vertices[v * 3 + 2]
				};

				// Apply model transform
				pos = Vector3Transform(pos, model.transform);

				// Apply uniform scale
				verts.push_back(pos.x * scale);
				verts.push_back(pos.y * scale);
				verts.push_back(pos.z * scale);
			}

			// Add indices
			if (mesh.indices != nullptr) {
				for (int i = 0; i < mesh.triangleCount * 3; i += 3) {
					tris.push_back(vertexOffset + mesh.indices[i]);
					tris.push_back(vertexOffset + mesh.indices[i + 1]);
					tris.push_back(vertexOffset + mesh.indices[i + 2]);
				}
			} else {
				// If no indices, assume sequential triangles
				for (int i = 0; i < mesh.vertexCount; i += 3) {
					tris.push_back(vertexOffset + i);
					tris.push_back(vertexOffset + i + 1);
					tris.push_back(vertexOffset + i + 2);
				}
			}
		}

		int nverts = verts.size() / 3;
		int ntris = tris.size() / 3;

		if (nverts == 0 || ntris == 0) {
			Core::Logger::errorLog("NavMesh: No geometry found.");
			return false;
		}

		// Calculate bounds
		float bmin[3] = { verts[0], verts[1], verts[2] };
		float bmax[3] = { verts[0], verts[1], verts[2] };
		for (int i = 1; i < nverts; ++i) {
			const float* v = &verts[i * 3];
			rcVmin(bmin, v);
			rcVmax(bmax, v);
		}

		// 2. Initialize Recast configuration
		rcConfig cfg;
		memset(&cfg, 0, sizeof(cfg));
		cfg.cs = 0.3f;          // Cell size
		cfg.ch = 0.2f;          // Cell height
		cfg.walkableSlopeAngle = 45.0f; // Standard slope for smoother ground navigation
		cfg.walkableHeight = (int)ceilf(1.5f / cfg.ch); // Allow lower ceilings
		cfg.walkableClimb = (int)floorf(1.0f / cfg.ch); // Allow higher steps
		cfg.walkableRadius = (int)ceilf(0.3f / cfg.cs); // Allow narrower gaps
		cfg.maxEdgeLen = (int)(12.0f / cfg.cs);
		cfg.maxSimplificationError = 1.3f;
		cfg.minRegionArea = (int)rcSqr(8);      // Note: area = size*size
		cfg.mergeRegionArea = (int)rcSqr(20);   // Note: area = size*size
		cfg.maxVertsPerPoly = 6;
		cfg.detailSampleDist = 6.0f < 0.9f ? 0 : cfg.cs * 6.0f;
		cfg.detailSampleMaxError = 1.0f;

		rcVcopy(cfg.bmin, bmin);
		rcVcopy(cfg.bmax, bmax);
		rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

		rcContext ctx;

		// 3. Rasterize input polygon soup
		rcHeightfield* solid = rcAllocHeightfield();
		if (!rcCreateHeightfield(&ctx, *solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch)) {
			Core::Logger::errorLog("NavMesh: Could not create solid heightfield.");
			return false;
		}

		// Allocate and populate triangle areas
		unsigned char* triareas = new unsigned char[ntris];
		memset(triareas, 0, ntris);
		rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, verts.data(), nverts, tris.data(), ntris, triareas);
		if (!rcRasterizeTriangles(&ctx, verts.data(), nverts, tris.data(), triareas, ntris, *solid, cfg.walkableClimb)) {
			Core::Logger::errorLog("NavMesh: Could not rasterize triangles.");
			delete[] triareas;
			return false;
		}
		delete[] triareas;

		// 4. Filter walkables surfaces
		rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *solid);
		rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid);
		rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *solid);

		// 5. Partition walkable surface to simple regions
		rcCompactHeightfield* chf = rcAllocCompactHeightfield();
		if (!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid, *chf)) {
			Core::Logger::errorLog("NavMesh: Could not build compact data.");
			return false;
		}
		rcFreeHeightField(solid);

		if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf)) {
			Core::Logger::errorLog("NavMesh: Could not erode.");
			return false;
		}

		if (!rcBuildDistanceField(&ctx, *chf)) {
			Core::Logger::errorLog("NavMesh: Could not build distance field.");
			return false;
		}

		if (!rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea)) {
			Core::Logger::errorLog("NavMesh: Could not build regions.");
			return false;
		}

		// 6. Trace and simplify region contours
		rcContourSet* cset = rcAllocContourSet();
		if (!rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset)) {
			Core::Logger::errorLog("NavMesh: Could not create contours.");
			return false;
		}

		// 7. Build polygons mesh from contours
		rcPolyMesh* pmesh = rcAllocPolyMesh();
		if (!rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh)) {
			Core::Logger::errorLog("NavMesh: Could not triangulate contours.");
			return false;
		}

		// 8. Create detail mesh
		rcPolyMeshDetail* dmesh = rcAllocPolyMeshDetail();
		if (!rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh)) {
			Core::Logger::errorLog("NavMesh: Could not build detail mesh.");
			return false;
		}
		rcFreeCompactHeightfield(chf);
		rcFreeContourSet(cset);

		// 9. Create Detour data
		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = pmesh->verts;
		params.vertCount = pmesh->nverts;
		params.polys = pmesh->polys;
		params.polyAreas = pmesh->areas;
		params.polyFlags = pmesh->flags;
		params.polyCount = pmesh->npolys;
		params.nvp = pmesh->nvp;
		params.detailMeshes = dmesh->meshes;
		params.detailVerts = dmesh->verts;
		params.detailVertsCount = dmesh->nverts;
		params.detailTris = dmesh->tris;
		params.detailTriCount = dmesh->ntris;
		params.walkableHeight = 2.0f;
		params.walkableRadius = 0.6f;
		params.walkableClimb = 0.6f;
		rcVcopy(params.bmin, pmesh->bmin);
		rcVcopy(params.bmax, pmesh->bmax);
		params.cs = cfg.cs;
		params.ch = cfg.ch;
		params.buildBvTree = true;

		// For all polys, set flags to 1 so they are considered walkable
		for (int i = 0; i < pmesh->npolys; ++i) {
			pmesh->flags[i] = 1;
		}

		unsigned char* navData = 0;
		int navDataSize = 0;
		if (!dtCreateNavMeshData(&params, &navData, &navDataSize)) {
			Core::Logger::errorLog("NavMesh: Could not build Detour navmesh.");
			return false;
		}

		_navMesh = dtAllocNavMesh();
		if (!_navMesh) {
			Core::Logger::errorLog("NavMesh: Could not allocate Detour navmesh.");
			dtFree(navData);
			return false;
		}

		dtStatus status = _navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
		if (dtStatusFailed(status)) {
			Core::Logger::errorLog("NavMesh: Could not init Detour navmesh.");
			dtFree(navData);
			return false;
		}

		_navQuery = dtAllocNavMeshQuery();
		status = _navQuery->init(_navMesh, 2048);
		if (dtStatusFailed(status)) {
			Core::Logger::errorLog("NavMesh: Could not init Detour navmesh query.");
			return false;
		}

		rcFreePolyMesh(pmesh);
		rcFreePolyMeshDetail(dmesh);

		Core::Logger::debugLog("NavMesh: Successfully built! Polys: " + std::to_string(params.polyCount));
		return true;
	}

	Vector3 NavMesh::getClosestWalkablePosition(Vector3 pos) const {
		if (!isReady()) return pos;

		dtQueryFilter filter;
		filter.setIncludeFlags(0xffff);
		filter.setExcludeFlags(0);

		float extents[3] = { 2.0f, 100.0f, 2.0f }; // Huge Y extent to snap 2D positions to 3D terrain
		float center[3] = { pos.x, pos.y, pos.z };

		dtPolyRef nearestRef = 0;
		float nearestPt[3];
		_navQuery->findNearestPoly(center, extents, &filter, &nearestRef, nearestPt);

		if (nearestRef) {
			return { nearestPt[0], nearestPt[1], nearestPt[2] };
		}
		return pos; // Fallback
	}

	std::vector<Vector2> NavMesh::findPath(Vector3 start, Vector3 end) const {
		if (!isReady()) return {};

		dtQueryFilter filter;
		filter.setIncludeFlags(0xffff);
		filter.setExcludeFlags(0);

		float extents[3] = { 2.0f, 100.0f, 2.0f }; // Huge Y extent
		
		float startPos[3] = { start.x, start.y, start.z }; 
		float endPos[3] = { end.x, end.y, end.z };

		dtPolyRef startRef, endRef;
		float startPt[3], endPt[3];

		_navQuery->findNearestPoly(startPos, extents, &filter, &startRef, startPt);
		_navQuery->findNearestPoly(endPos, extents, &filter, &endRef, endPt);

		if (!startRef || !endRef) {
			Core::Logger::debugLog("NavMesh::findPath - Failed to find startRef (" + std::to_string(startRef) + ") or endRef (" + std::to_string(endRef) + ")");
			return {};
		}

		dtPolyRef path[256];
		int pathCount = 0;
		_navQuery->findPath(startRef, endRef, startPt, endPt, &filter, path, &pathCount, 256);

		// Only accept the path if it reaches the destination polygon
		if (pathCount > 0 && path[pathCount - 1] == endRef) {
			float straightPath[256 * 3];
			unsigned char straightPathFlags[256];
			dtPolyRef straightPathPolys[256];
			int straightPathCount = 0;

			_navQuery->findStraightPath(startPt, endPt, path, pathCount,
				straightPath, straightPathFlags, straightPathPolys, &straightPathCount, 256);

			if (straightPathCount > 0) {
				// Final check on the XZ plane only.
				// Large height differences are valid on reachable hills and stairs,
				// so using full 3D distance rejects good paths to elevated points.
				float lx = straightPath[(straightPathCount - 1) * 3 + 0];
				float lz = straightPath[(straightPathCount - 1) * 3 + 2];
				float dx = lx - end.x;
				float dz = lz - end.z;
				float horizontal_dist_sq = dx * dx + dz * dz;

				if (horizontal_dist_sq > k_max_path_end_distance_sq) {
					Core::Logger::debugLog(
						"NavMesh::findPath - Path ends too far from click on XZ plane (" +
						std::to_string(std::sqrt(horizontal_dist_sq)) + "m), rejecting.");
					return {};
				}

				std::vector<Vector2> result;
				for (int i = 0; i < straightPathCount; ++i) {
					result.push_back({ straightPath[i * 3], straightPath[i * 3 + 2] });
				}
				Core::Logger::debugLog("NavMesh::findPath - Found complete path with " + std::to_string(straightPathCount) + " points");
				return result;
			}
		}

		Core::Logger::debugLog("NavMesh::findPath - Path incomplete or destination unreachable");
		return {};
	}

} // namespace Nawia::World
