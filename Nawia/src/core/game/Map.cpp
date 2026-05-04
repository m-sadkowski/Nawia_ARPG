#include "Map.h"
#include "Logger.h"

#include <raymath.h>
#include <filesystem>

namespace Nawia::Core {

// =============================================================================
// Constructor / Destructor
// =============================================================================

Map::Map(ResourceManager& resource_manager) 
	: _resource_manager(resource_manager) 
{
}

Map::~Map()
{
	if (_model_loaded)
	{
		UnloadModel(_model);
	}
}

// =============================================================================
// Loading
// =============================================================================

void Map::loadMap(const std::string& filename, const float scale, const Vector3 offset, const Vector3 rotation) 
{
	if (_model_loaded) {
		UnloadModel(_model);
		_model_loaded = false;
	}

	const std::string filepath = "../assets/maps/" + filename;
	_scale = scale;
	_offset = offset;
	_rotation = rotation;
	
	if (!std::filesystem::exists(filepath))
	{
		Logger::errorLog("Map::loadMap - File not found: " + filepath + ", loading placeholder");
		loadPlaceholder();
		return;
	}

	_model = LoadModel(filepath.c_str());

	if (_model.meshCount == 0)
	{
		Logger::errorLog("Map::loadMap - Failed to load model: " + filepath + ", loading placeholder");
		loadPlaceholder();
		return;
	}

	_model_loaded = true;
	_is_placeholder = false;

	// Apply rotation to model transform (Euler angles in degrees -> radians)
	Matrix matRotX = MatrixRotateX(_rotation.x * DEG2RAD);
	Matrix matRotY = MatrixRotateY(_rotation.y * DEG2RAD);
	Matrix matRotZ = MatrixRotateZ(_rotation.z * DEG2RAD);
	_model.transform = MatrixMultiply(MatrixMultiply(matRotX, matRotY), matRotZ);

	// Build navmesh
	_navmesh.buildFromModel(_model, _scale);

	// Cache bounding boxes for optimized raycasting
	_mesh_bboxes.clear();
	_mesh_bboxes.reserve(_model.meshCount);
	for (int i = 0; i < _model.meshCount; i++) {
		_mesh_bboxes.push_back(GetMeshBoundingBox(_model.meshes[i]));
	}

	Logger::debugLog("Map loaded: " + filename + " (meshes=" + std::to_string(_model.meshCount) + ", scale=" + std::to_string(_scale) + ", offset={" + std::to_string(_offset.x) + ", " + std::to_string(_offset.y) + ", " + std::to_string(_offset.z) + "}, rotation={" + std::to_string(_rotation.x) + ", " + std::to_string(_rotation.y) + ", " + std::to_string(_rotation.z) + "})");
}

void Map::loadPlaceholder()
{
	// Generate a large flat plane as a placeholder ground
	const Mesh plane_mesh = GenMeshPlane(60.0f, 60.0f, 10, 10);
	_model = LoadModelFromMesh(plane_mesh);

	// Give it a nice dark green color via material
	_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = Color{45, 55, 40, 255};

	_model_loaded = true;
	_is_placeholder = true;

	_mesh_bboxes.clear();
	_mesh_bboxes.push_back(GetMeshBoundingBox(_model.meshes[0]));

	Logger::debugLog("Map placeholder loaded (60x60 plane)");
}

// =============================================================================
// Rendering
// =============================================================================

void Map::render() const
{
	if (!_model_loaded) return;

	DrawModel(_model, _offset, _scale, WHITE);

	if (_is_placeholder)
	{
		// Draw a grid on the placeholder for spatial reference
		DrawGrid(60, 1.0f);
	}
}

RayCollision Map::getRayCollision(Ray ray) const
{
	if (!_model_loaded) return { false, 0, {0,0,0}, {0,0,0} };
	
	if (_is_placeholder) {
		return GetRayCollisionQuad(ray, 
			{ -1000.0f, 0.0f, -1000.0f }, 
			{ -1000.0f, 0.0f, 1000.0f }, 
			{ 1000.0f, 0.0f, 1000.0f }, 
			{ 1000.0f, 0.0f, -1000.0f });
	}

	RayCollision closest = { false, 1000000.0f, {0,0,0}, {0,0,0} };

	// Create a transform matrix that matches how the model is rendered
	Matrix matTransform = _model.transform;
	matTransform = MatrixMultiply(matTransform, MatrixScale(_scale, _scale, _scale));
	matTransform = MatrixMultiply(matTransform, MatrixTranslate(_offset.x, _offset.y, _offset.z));

	for (int i = 0; i < _model.meshCount; i++) {
		// Optimization: Check transformed bounding box first
		BoundingBox bb = _mesh_bboxes[i];
		// Transform bounding box to world space
		Vector3 corners[8] = {
			{ bb.min.x, bb.min.y, bb.min.z }, { bb.max.x, bb.min.y, bb.min.z },
			{ bb.min.x, bb.max.y, bb.min.z }, { bb.max.x, bb.max.y, bb.min.z },
			{ bb.min.x, bb.min.y, bb.max.z }, { bb.max.x, bb.min.y, bb.max.z },
			{ bb.min.x, bb.max.y, bb.max.z }, { bb.max.x, bb.max.y, bb.max.z }
		};
		BoundingBox worldBB = { {1e9, 1e9, 1e9}, {-1e9, -1e9, -1e9} };
		for (int j = 0; j < 8; j++) {
			Vector3 v = Vector3Transform(corners[j], matTransform);
			worldBB.min = Vector3Min(worldBB.min, v);
			worldBB.max = Vector3Max(worldBB.max, v);
		}

		if (!GetRayCollisionBox(ray, worldBB).hit) continue;

		RayCollision hit = GetRayCollisionMesh(ray, _model.meshes[i], matTransform);
		if (hit.hit && hit.distance < closest.distance) {
			closest = hit;
		}
	}

	return closest;
}

// =============================================================================
// Walkability 
// =============================================================================

bool Map::isWalkable(float world_x, float world_z) const
{
	if (!_navmesh.isReady()) return true;

	Vector3 pos = { world_x, 0.0f, world_z };
	Vector3 closest = _navmesh.getClosestWalkablePosition(pos);
	
	// If the closest walkable position is very close on the XZ plane, it's walkable
	float dx = closest.x - world_x;
	float dz = closest.z - world_z;
	return (dx*dx + dz*dz) < 0.5f;
}

// =============================================================================
// Pathfinding
// =============================================================================

std::vector<Vector2> Map::findPath(Vector3 start, Vector3 end) const
{
	if (!_navmesh.isReady()) return { {start.x, start.z}, {end.x, end.z} };
	return _navmesh.findPath(start, end);
}

} // namespace Nawia::Core