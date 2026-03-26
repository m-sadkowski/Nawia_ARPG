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

void Map::loadMap(const std::string& filename) 
{
	const std::string filepath = "../assets/maps/" + filename;
	
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

	Logger::debugLog("Map loaded: " + filename + " (meshes=" + std::to_string(_model.meshCount) + ")");
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

	DrawModel(_model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);

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