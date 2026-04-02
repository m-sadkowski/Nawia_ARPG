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

// =============================================================================
// Walkability (stub — always walkable)
// =============================================================================

bool Map::isWalkable(float world_x, float world_z) const
{
	return true;
}

// =============================================================================
// Pathfinding (stub — returns empty)
// =============================================================================

std::vector<Vector2> Map::findPath(Vector2 start, Vector2 end) const
{
	return {};
}

} // namespace Nawia::Core