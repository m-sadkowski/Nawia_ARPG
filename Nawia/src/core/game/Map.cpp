#include "Map.h"
#include "Logger.h"

#include <tinyxml2.h>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Nawia::Core {

// =============================================================================
// Constructor
// =============================================================================

Map::Map(ResourceManager& resource_manager) 
	: _resource_manager(resource_manager) 
{
}

// =============================================================================
// Public API
// =============================================================================

void Map::loadMap(const std::string& filename) 
{
	const std::string filepath = "../assets/maps/" + filename;
	std::ifstream file(filepath);
	
	if (!file.is_open()) {
		Logger::errorLog("Map::loadMap - Cannot open: " + filename);
		return;
	}

	_map_base_path = fs::path(filepath).parent_path().string() + "/";

	json map_data;
	try {
		file >> map_data;
	} catch (const json::parse_error&) {
		Logger::errorLog("Map::loadMap - Parse error: " + filename);
		return;
	}

	if (!loadTilesets(map_data)) {
		Logger::errorLog("Map::loadMap - Failed to load tilesets");
		return;
	}

	int min_x, min_y, max_x, max_y;
	min_x = min_y = std::numeric_limits<int>::max();
	max_x = max_y = std::numeric_limits<int>::lowest();

	const auto& layers = map_data["layers"];
	calculateMapBounds(layers, min_x, min_y, max_x, max_y);

	if (min_x == std::numeric_limits<int>::max()) {
		Logger::errorLog("Map::loadMap - Empty map");
		return;
	}

	_offset_x = min_x;
	_offset_y = min_y;
	initializeGrids(max_x - min_x, max_y - min_y);
	loadLayers(layers, map_data);

	Logger::debugLog("Map loaded: " + filename + 
		" (offset=" + std::to_string(_offset_x) + "," + std::to_string(_offset_y) + 
		", tilesets=" + std::to_string(_tilesets.size()) + ")");
}

void Map::render(const float offset_x, const float offset_y) 
{
	renderLayer(_layer_ground, offset_x, offset_y);
	renderLayer(_layer_on_ground, offset_x, offset_y);
	renderLayer(_layer_over_ground, offset_x, offset_y);
	
	if (_debug_walkability)
		renderWalkabilityDebug(offset_x, offset_y);
}

bool Map::isWalkable(const float world_x, const float world_y) const 
{
	const int grid_x = static_cast<int>(std::floor(world_x)) - _offset_x;
	const int grid_y = static_cast<int>(std::floor(world_y)) - _offset_y;

	if (grid_y < 0 || grid_y >= static_cast<int>(_walkability_grid.size()))
		return false;
	if (grid_x < 0 || grid_x >= static_cast<int>(_walkability_grid[grid_y].size()))
		return false;

	return _walkability_grid[grid_y][grid_x];
}

// =============================================================================
// Tileset Loading
// =============================================================================

bool Map::loadTilesets(const json& map_data)
{
	if (!map_data.contains("tilesets")) {
		Logger::errorLog("Map has no tilesets");
		return false;
	}

	_tilesets.clear();
	_tile_textures.clear();
	_tile_source_rects.clear();

	for (const auto& ts : map_data["tilesets"]) {
		TilesetInfo tileset;
		tileset.firstgid = ts["firstgid"];
		tileset.source = ts["source"];

		const std::string tsx_path = _map_base_path + tileset.source;
		if (!loadTilesetFile(tsx_path, tileset)) {
			Logger::errorLog("Failed to load tileset: " + tsx_path);
			return false;
		}

		_tilesets.push_back(tileset);
	}

	return true;
}

bool Map::loadTilesetFile(const std::string& tsx_path, TilesetInfo& tileset)
{
	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(tsx_path.c_str()) != tinyxml2::XML_SUCCESS) {
		Logger::errorLog("Cannot parse TSX: " + tsx_path);
		return false;
	}

	auto* root = doc.FirstChildElement("tileset");
	if (!root) {
		Logger::errorLog("No <tileset> in: " + tsx_path);
		return false;
	}

	root->QueryIntAttribute("tilecount", &tileset.tilecount);
	const std::string tsx_dir = fs::path(tsx_path).parent_path().string() + "/";

	// --- Load single-image tileset (spritesheet) ---
	auto* image_elem = root->FirstChildElement("image");
	if (image_elem && !root->FirstChildElement("tile")) {
		const char* img_source = image_elem->Attribute("source");
		if (img_source) {
			const std::string full_path = tsx_dir + img_source;
			auto texture = _resource_manager.getTexture(full_path);
			
			if (!texture) {
				Logger::errorLog("Cannot load tileset image: " + full_path);
				return false;
			}

			const int columns = texture->width / TILE_WIDTH;

			for (int i = 0; i < tileset.tilecount; i++) {
				_tile_textures[tileset.firstgid + i] = texture;

				const int x = (i % columns) * TILE_WIDTH;
				const int y = (i / columns) * TILE_HEIGHT;
				_tile_source_rects[tileset.firstgid + i] = Rectangle{
					static_cast<float>(x), 
					static_cast<float>(y), 
					static_cast<float>(TILE_WIDTH), 
					static_cast<float>(TILE_HEIGHT)
				};
			}
		}
	}

	// --- Load individual tile elements ---
	for (auto* tile = root->FirstChildElement("tile"); tile; tile = tile->NextSiblingElement("tile")) {
		int local_id = 0;
		tile->QueryIntAttribute("id", &local_id);
		const int gid = tileset.firstgid + local_id;

		// Load tile image (for image collections)
		if (auto* tile_image = tile->FirstChildElement("image")) {
			if (const char* src = tile_image->Attribute("source")) {
				if (auto tex = _resource_manager.getTexture(tsx_dir + src)) {
					_tile_textures[gid] = tex;
					_tile_source_rects[gid] = Rectangle{
						0.0f, 0.0f, 
						static_cast<float>(tex->width), 
						static_cast<float>(tex->height)
					};
				}
			}
		}

		// Load tile properties
		if (auto* properties = tile->FirstChildElement("properties")) {
			for (auto* prop = properties->FirstChildElement("property"); prop; prop = prop->NextSiblingElement("property")) {
				const char* name = prop->Attribute("name");
				if (name && std::string(name) == "isWalkable") {
					const char* value = prop->Attribute("value");
					tileset.tile_walkable[local_id] = (value && std::string(value) == "true");
				}
			}
		}
	}

	return true;
}

// =============================================================================
// Layer Loading
// =============================================================================

void Map::calculateMapBounds(const json& layers, int& min_x, int& min_y, int& max_x, int& max_y)
{
	for (const auto& layer : layers) {
		if (layer["type"] != "tilelayer" || !layer.contains("chunks"))
			continue;

		for (const auto& chunk : layer["chunks"]) {
			const int cx = chunk["x"], cy = chunk["y"];
			const int cw = chunk["width"], ch = chunk["height"];

			min_x = std::min(min_x, cx);
			min_y = std::min(min_y, cy);
			max_x = std::max(max_x, cx + cw);
			max_y = std::max(max_y, cy + ch);
		}
	}
}

void Map::initializeGrids(int width, int height)
{
	_layer_ground.assign(height, std::vector<int>(width, 0));
	_layer_on_ground.assign(height, std::vector<int>(width, 0));
	_layer_over_ground.assign(height, std::vector<int>(width, 0));
	_walkability_grid.assign(height, std::vector<bool>(width, true));
}

void Map::loadLayers(const json& layers, const json& map_data)
{
	for (const auto& layer : layers) {
		const std::string type = layer["type"];
		
		if (type == "tilelayer" && layer.contains("chunks")) {
			const std::string name = layer["name"];
			
			if (name == "ground")
				loadTileLayer(layer, _layer_ground);
			else if (name == "on_ground")
				loadTileLayer(layer, _layer_on_ground);
			else if (name == "over_ground")
				loadTileLayer(layer, _layer_over_ground);
			else if (name == "walkability")
				loadWalkabilityLayer(layer);
		}
		else if (type == "objectgroup") {
			loadObjectLayer(layer, map_data);
		}
	}
}

void Map::loadTileLayer(const json& layer, std::vector<std::vector<int>>& grid)
{
	for (const auto& chunk : layer["chunks"]) {
		const int chunk_x = chunk["x"], chunk_y = chunk["y"];
		const int width = chunk["width"], height = chunk["height"];
		const auto& data = chunk["data"];

		int idx = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x, ++idx) {
				const int gid = data[idx];
				if (gid <= 0) continue;

				const int gx = chunk_x + x - _offset_x;
				const int gy = chunk_y + y - _offset_y;

				if (gy >= 0 && gy < static_cast<int>(grid.size()) &&
					gx >= 0 && gx < static_cast<int>(grid[gy].size())) {
					grid[gy][gx] = gid;
					if (!getWalkableForGID(gid)) {
						_walkability_grid[gy][gx] = false;
					}
				}
			}
		}
	}
}

void Map::loadWalkabilityLayer(const json& layer)
{
	for (const auto& chunk : layer["chunks"]) {
		const int chunk_x = chunk["x"], chunk_y = chunk["y"];
		const int width = chunk["width"], height = chunk["height"];
		const auto& data = chunk["data"];

		int idx = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x, ++idx) {
				const int gid = data[idx];
				if (gid <= 0) continue;

				const int gx = chunk_x + x - _offset_x;
				const int gy = chunk_y + y - _offset_y;

				if (gy >= 0 && gy < static_cast<int>(_walkability_grid.size()) &&
					gx >= 0 && gx < static_cast<int>(_walkability_grid[gy].size())) {
					if (!getWalkableForGID(gid)) {
						_walkability_grid[gy][gx] = false;
					}
				}
			}
		}
	}
}

void Map::loadObjectLayer(const json& layer, const json& map_data)
{
	if (!layer.contains("objects"))
		return;

	const float tile_height = map_data.value("tileheight", 64.0f);

	for (const auto& obj : layer["objects"]) {
		const std::string name = obj["name"];
		
		if (name == "playerspawn") {
			_player_spawn_pos.x = obj["x"].get<float>() / tile_height;
			_player_spawn_pos.y = obj["y"].get<float>() / tile_height;
			Logger::debugLog("Player spawn: " + 
				std::to_string(_player_spawn_pos.x) + ", " + 
				std::to_string(_player_spawn_pos.y));
		}
	}
}

// =============================================================================
// Rendering
// =============================================================================

void Map::renderLayer(const std::vector<std::vector<int>>& layer, float offset_x, float offset_y)
{
	for (size_t y = 0; y < layer.size(); ++y) {
		for (size_t x = 0; x < layer[y].size(); ++x) {
			const int gid = layer[y][x];
			if (gid > 0) {
				renderTile(gid, 
					static_cast<int>(x) + _offset_x, 
					static_cast<int>(y) + _offset_y, 
					offset_x, offset_y);
			}
		}
	}
}

void Map::renderTile(int gid, int world_x, int world_y, float offset_x, float offset_y)
{
	auto texture = getTextureForGID(gid);
	if (!texture) return;

	const Vector2 iso = worldToIso(world_x, world_y, offset_x, offset_y);
	const Texture2D* tex = texture.get();
	
	Rectangle source;
	auto rect_it = _tile_source_rects.find(gid);
	if (rect_it != _tile_source_rects.end()) {
		source = rect_it->second;
	} else {
		// Fallback (should not happen if loaded correctly)
		source = {0, 0, static_cast<float>(tex->width), static_cast<float>(tex->height)};
	}
	
	const Rectangle dest = {iso.x, iso.y, 
		static_cast<float>(source.width), 
		static_cast<float>(source.height)};
	
	// Anchor at diamond base: center horizontally, (height - TILE_HEIGHT) from top
	const Vector2 origin = {
		static_cast<float>(source.width) / 2.0f,
		static_cast<float>(source.height - TILE_HEIGHT)
	};
	
	DrawTexturePro(*tex, source, dest, origin, 0.0f, WHITE);
}

void Map::renderWalkabilityDebug(float offset_x, float offset_y)
{
	constexpr float half_w = TILE_WIDTH / 2.0f;
	constexpr float half_h = TILE_HEIGHT / 2.0f;

	for (size_t y = 0; y < _walkability_grid.size(); ++y) {
		for (size_t x = 0; x < _walkability_grid[y].size(); ++x) {
			const Vector2 iso = worldToIso(
				static_cast<int>(x) + _offset_x,
				static_cast<int>(y) + _offset_y,
				offset_x, offset_y);

			const Vector2 top    = {iso.x, iso.y};
			const Vector2 right  = {iso.x + half_w, iso.y + half_h};
			const Vector2 bottom = {iso.x, iso.y + TILE_HEIGHT};
			const Vector2 left   = {iso.x - half_w, iso.y + half_h};

			const Color color = _walkability_grid[y][x] 
				? Color{0, 255, 0, 50}    // Green = walkable
				: Color{255, 0, 0, 50};   // Red = blocked

			DrawTriangle(top, left, bottom, color);
			DrawTriangle(top, bottom, right, color);
		}
	}
}

// =============================================================================
// Utilities
// =============================================================================

std::shared_ptr<Texture2D> Map::getTextureForGID(int gid) const
{
	auto it = _tile_textures.find(gid);
	return (it != _tile_textures.end()) ? it->second : nullptr;
}

bool Map::getWalkableForGID(int gid) const
{
	// Find tileset containing this GID (search from highest firstgid)
	for (auto it = _tilesets.rbegin(); it != _tilesets.rend(); ++it) {
		if (gid >= it->firstgid) {
			const int local_id = gid - it->firstgid;
			auto prop = it->tile_walkable.find(local_id);
			return (prop != it->tile_walkable.end()) ? prop->second : true;
		}
	}
	return true;
}

Vector2 Map::worldToIso(int world_x, int world_y, float offset_x, float offset_y) const
{
	return {
		(world_x - world_y) * (TILE_WIDTH / 2.0f) + offset_x,
		(world_x + world_y) * (TILE_HEIGHT / 2.0f) + offset_y
	};
}

} // namespace Nawia::Core