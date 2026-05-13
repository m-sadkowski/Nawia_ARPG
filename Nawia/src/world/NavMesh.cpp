#include "NavMesh.h"

#include <Logger.h>

#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <raymath.h>

#include <cmath>
#include <cstring>
#include <memory>
#include <string>

namespace Nawia::World {

	namespace {

		constexpr float k_max_path_end_distance_sq = 1.5f;
		constexpr float k_disabled_min_walkable_height = -9999.0f;

		float getTriangleAverageHeight(const std::vector<float>& verts, const std::vector<int>& tris, const int triangle_index) {
			const int tri_offset = triangle_index * 3;
			const int vertex_a = tris[tri_offset] * 3 + 1;
			const int vertex_b = tris[tri_offset + 1] * 3 + 1;
			const int vertex_c = tris[tri_offset + 2] * 3 + 1;
			return (verts[vertex_a] + verts[vertex_b] + verts[vertex_c]) / 3.0f;
		}

		struct HeightfieldDeleter {
			void operator()(rcHeightfield* value) const { rcFreeHeightField(value); }
		};

		struct CompactHeightfieldDeleter {
			void operator()(rcCompactHeightfield* value) const { rcFreeCompactHeightfield(value); }
		};

		struct ContourSetDeleter {
			void operator()(rcContourSet* value) const { rcFreeContourSet(value); }
		};

		struct PolyMeshDeleter {
			void operator()(rcPolyMesh* value) const { rcFreePolyMesh(value); }
		};

		struct PolyMeshDetailDeleter {
			void operator()(rcPolyMeshDetail* value) const { rcFreePolyMeshDetail(value); }
		};

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

	bool NavMesh::buildFromModel(const Model& model, float scale, Vector3 offset) {
		cleanup();

		if (model.meshCount == 0) {
			Core::Logger::errorLog("NavMesh: nie mozna zbudowac z pustego modelu.");
			return false;
		}

		Core::Logger::debugLog("NavMesh: budowanie z modelu, liczba meshy: " + std::to_string(model.meshCount));

		std::vector<float> verts;
		std::vector<int> tris;

		for (int mesh_index = 0; mesh_index < model.meshCount; ++mesh_index) {
			const Mesh& mesh = model.meshes[mesh_index];
			const int vertex_offset = static_cast<int>(verts.size() / 3);

			for (int vertex_index = 0; vertex_index < mesh.vertexCount; ++vertex_index) {
				Vector3 position = {
					mesh.vertices[vertex_index * 3 + 0],
					mesh.vertices[vertex_index * 3 + 1],
					mesh.vertices[vertex_index * 3 + 2]
				};

				position = Vector3Transform(position, model.transform);

				// Skala i offset musza odpowiadac temu, jak mapa jest renderowana.
				verts.push_back(position.x * scale + offset.x);
				verts.push_back(position.y * scale + offset.y);
				verts.push_back(position.z * scale + offset.z);
			}

			if (mesh.indices != nullptr) {
				for (int index = 0; index < mesh.triangleCount * 3; index += 3) {
					tris.push_back(vertex_offset + mesh.indices[index]);
					tris.push_back(vertex_offset + mesh.indices[index + 1]);
					tris.push_back(vertex_offset + mesh.indices[index + 2]);
				}
			} else {
				// Mesh bez indeksow traktujemy jako kolejne trojkaty.
				for (int index = 0; index < mesh.vertexCount; index += 3) {
					tris.push_back(vertex_offset + index);
					tris.push_back(vertex_offset + index + 1);
					tris.push_back(vertex_offset + index + 2);
				}
			}
		}

		const int nverts = static_cast<int>(verts.size() / 3);
		const int ntris = static_cast<int>(tris.size() / 3);

		if (nverts == 0 || ntris == 0) {
			Core::Logger::errorLog("NavMesh: nie znaleziono geometrii.");
			return false;
		}

		float bmin[3] = {verts[0], verts[1], verts[2]};
		float bmax[3] = {verts[0], verts[1], verts[2]};
		for (int vertex_index = 1; vertex_index < nverts; ++vertex_index) {
			const float* vertex = &verts[vertex_index * 3];
			rcVmin(bmin, vertex);
			rcVmax(bmax, vertex);
		}

		rcConfig cfg;
		memset(&cfg, 0, sizeof(cfg));
		cfg.cs = 0.3f;
		cfg.ch = 0.2f;
		cfg.walkableSlopeAngle = 45.0f;
		cfg.walkableHeight = static_cast<int>(ceilf(1.5f / cfg.ch));
		cfg.walkableClimb = static_cast<int>(floorf(1.0f / cfg.ch));
		cfg.walkableRadius = static_cast<int>(ceilf(0.3f / cfg.cs));
		cfg.maxEdgeLen = static_cast<int>(12.0f / cfg.cs);
		cfg.maxSimplificationError = 1.3f;
		cfg.minRegionArea = static_cast<int>(rcSqr(8));
		cfg.mergeRegionArea = static_cast<int>(rcSqr(20));
		cfg.maxVertsPerPoly = 6;
		cfg.detailSampleDist = 6.0f < 0.9f ? 0 : cfg.cs * 6.0f;
		cfg.detailSampleMaxError = 1.0f;

		rcVcopy(cfg.bmin, bmin);
		rcVcopy(cfg.bmax, bmax);
		rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

		rcContext ctx;

		std::unique_ptr<rcHeightfield, HeightfieldDeleter> solid(rcAllocHeightfield());
		if (!solid || !rcCreateHeightfield(&ctx, *solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch)) {
			Core::Logger::errorLog("NavMesh: nie udalo sie stworzyc heightfield.");
			return false;
		}

		std::vector<unsigned char> triangle_areas(ntris, 0);
		rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, verts.data(), nverts, tris.data(), ntris, triangle_areas.data());
		if (_min_walkable_height > k_disabled_min_walkable_height) {
			for (int triangle_index = 0; triangle_index < ntris; ++triangle_index) {
				if (triangle_areas[triangle_index] == RC_NULL_AREA)
					continue;

				if (getTriangleAverageHeight(verts, tris, triangle_index) < _min_walkable_height)
					triangle_areas[triangle_index] = RC_NULL_AREA;
			}
		}

		if (!rcRasterizeTriangles(&ctx, verts.data(), nverts, tris.data(), triangle_areas.data(), ntris, *solid, cfg.walkableClimb)) {
			Core::Logger::errorLog("NavMesh: nie udalo sie zrasteryzowac trojkatow.");
			return false;
		}

		rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *solid);
		rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid);
		rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *solid);

		std::unique_ptr<rcCompactHeightfield, CompactHeightfieldDeleter> chf(rcAllocCompactHeightfield());
		if (!chf || !rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid, *chf)) {
			Core::Logger::errorLog("NavMesh: nie udalo sie zbudowac compact heightfield.");
			return false;
		}
		solid.reset();

		if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf)) {
			Core::Logger::errorLog("NavMesh: nie udalo sie zwezyc obszaru chodzenia.");
			return false;
		}

		if (!rcBuildDistanceField(&ctx, *chf)) {
			Core::Logger::errorLog("NavMesh: nie udalo sie zbudowac distance field.");
			return false;
		}

		if (!rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea)) {
			Core::Logger::errorLog("NavMesh: nie udalo sie zbudowac regionow.");
			return false;
		}

		std::unique_ptr<rcContourSet, ContourSetDeleter> cset(rcAllocContourSet());
		if (!cset || !rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset)) {
			Core::Logger::errorLog("NavMesh: nie udalo sie stworzyc konturow.");
			return false;
		}

		std::unique_ptr<rcPolyMesh, PolyMeshDeleter> pmesh(rcAllocPolyMesh());
		if (!pmesh || !rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh)) {
			Core::Logger::errorLog("NavMesh: nie udalo sie zbudowac poly mesh.");
			return false;
		}

		std::unique_ptr<rcPolyMeshDetail, PolyMeshDetailDeleter> dmesh(rcAllocPolyMeshDetail());
		if (!dmesh || !rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh)) {
			Core::Logger::errorLog("NavMesh: nie udalo sie zbudowac detail mesh.");
			return false;
		}
		chf.reset();
		cset.reset();

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

		// Wszystkie poligony zbudowane przez Recast traktujemy jako chodliwe.
		for (int poly_index = 0; poly_index < pmesh->npolys; ++poly_index) {
			pmesh->flags[poly_index] = 1;
		}

		unsigned char* nav_data = 0;
		int nav_data_size = 0;
		if (!dtCreateNavMeshData(&params, &nav_data, &nav_data_size)) {
			Core::Logger::errorLog("NavMesh: nie udalo sie zbudowac danych Detour.");
			return false;
		}

		dtNavMesh* nav_mesh = dtAllocNavMesh();
		if (!nav_mesh) {
			Core::Logger::errorLog("NavMesh: nie udalo sie zaalokowac Detour navmesh.");
			dtFree(nav_data);
			return false;
		}

		dtStatus status = nav_mesh->init(nav_data, nav_data_size, DT_TILE_FREE_DATA);
		if (dtStatusFailed(status)) {
			Core::Logger::errorLog("NavMesh: nie udalo sie zainicjalizowac Detour navmesh.");
			dtFree(nav_data);
			dtFreeNavMesh(nav_mesh);
			return false;
		}

		dtNavMeshQuery* nav_query = dtAllocNavMeshQuery();
		if (!nav_query) {
			Core::Logger::errorLog("NavMesh: nie udalo sie zaalokowac Detour navmesh query.");
			dtFreeNavMesh(nav_mesh);
			return false;
		}

		status = nav_query->init(nav_mesh, 2048);
		if (dtStatusFailed(status)) {
			Core::Logger::errorLog("NavMesh: nie udalo sie zainicjalizowac zapytan Detour.");
			dtFreeNavMeshQuery(nav_query);
			dtFreeNavMesh(nav_mesh);
			return false;
		}

		_navMesh = nav_mesh;
		_navQuery = nav_query;

		Core::Logger::debugLog("NavMesh: zbudowano pomyslnie. Poligony: " + std::to_string(params.polyCount));
		return true;
	}

	Vector3 NavMesh::getClosestWalkablePosition(Vector3 pos) const {
		if (!isReady()) return pos;

		dtQueryFilter filter;
		filter.setIncludeFlags(0xffff);
		filter.setExcludeFlags(0);

		float extents[3] = {2.0f, 100.0f, 2.0f}; // Duzy zasieg Y sciaga pozycje 2D na teren.
		float center[3] = {pos.x, pos.y, pos.z};

		dtPolyRef nearest_ref = 0;
		float nearest_point[3];
		_navQuery->findNearestPoly(center, extents, &filter, &nearest_ref, nearest_point);

		if (nearest_ref) {
			return {nearest_point[0], nearest_point[1], nearest_point[2]};
		}
		return pos;
	}

	std::vector<Vector2> NavMesh::findPath(Vector3 start, Vector3 end) const {
		if (!isReady()) return {};

		dtQueryFilter filter;
		filter.setIncludeFlags(0xffff);
		filter.setExcludeFlags(0);

		float extents[3] = {2.0f, 100.0f, 2.0f};

		float start_pos[3] = {start.x, start.y, start.z};
		float end_pos[3] = {end.x, end.y, end.z};

		dtPolyRef start_ref = 0;
		dtPolyRef end_ref = 0;
		float start_point[3], end_point[3];

		_navQuery->findNearestPoly(start_pos, extents, &filter, &start_ref, start_point);
		_navQuery->findNearestPoly(end_pos, extents, &filter, &end_ref, end_point);

		if (!start_ref || !end_ref) {
			Core::Logger::debugLog("NavMesh::findPath - nie znaleziono start_ref (" +
				std::to_string(start_ref) + ") albo end_ref (" + std::to_string(end_ref) + ")");
			return {};
		}

		dtPolyRef path[256];
		int path_count = 0;
		_navQuery->findPath(start_ref, end_ref, start_point, end_point, &filter, path, &path_count, 256);

		// Akceptujemy tylko sciezke, ktora dochodzi do poligonu celu.
		if (path_count > 0 && path[path_count - 1] == end_ref) {
			float straight_path[256 * 3];
			unsigned char straight_path_flags[256];
			dtPolyRef straight_path_polys[256];
			int straight_path_count = 0;

			_navQuery->findStraightPath(start_point, end_point, path, path_count,
				straight_path, straight_path_flags, straight_path_polys, &straight_path_count, 256);

			if (straight_path_count > 0) {
				// Koncowy test robimy po XZ, bo wysokosc na wzgorzach moze byc poprawna.
				const float last_x = straight_path[(straight_path_count - 1) * 3 + 0];
				const float last_z = straight_path[(straight_path_count - 1) * 3 + 2];
				const float dx = last_x - end.x;
				const float dz = last_z - end.z;
				const float horizontal_distance_sq = dx * dx + dz * dz;

				if (horizontal_distance_sq > k_max_path_end_distance_sq) {
					Core::Logger::debugLog(
						"NavMesh::findPath - sciezka konczy sie za daleko od celu po XZ (" +
						std::to_string(std::sqrt(horizontal_distance_sq)) + "m), odrzucono.");
					return {};
				}

				std::vector<Vector2> result;
				for (int point_index = 0; point_index < straight_path_count; ++point_index) {
					result.push_back({straight_path[point_index * 3], straight_path[point_index * 3 + 2]});
				}
				Core::Logger::debugLog("NavMesh::findPath - znaleziono sciezke, punkty: " +
					std::to_string(straight_path_count));
				return result;
			}
		}

		Core::Logger::debugLog("NavMesh::findPath - sciezka niepelna albo cel nieosiagalny");
		return {};
	}

} // namespace Nawia::World
