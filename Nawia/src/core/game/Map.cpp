#include "Map.h"
#include "Logger.h"

#include <tinyxml2.h>
#include <fstream>
#include <filesystem>
#include <queue>
#include <algorithm>
#include <cmath>

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

	int min_x = std::numeric_limits<int>::max();
	int min_y = std::numeric_limits<int>::max();
	int max_x = std::numeric_limits<int>::lowest();
	int max_y = std::numeric_limits<int>::lowest();

	const auto& layers = map_data["layers"];
	calculateMapBounds(layers, min_x, min_y, max_x, max_y);

	const int width = max_x - min_x;
	const int height = max_y - min_y;

	_offset_x = min_x;
	_offset_y = min_y;
	initializeGrids(width, height);
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
		if (const char* img_source = image_elem->Attribute("source")) {
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

void Map::initializeGrids(const int width, const int height)
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

void Map::renderLayer(const std::vector<std::vector<int>>& layer, const float offset_x, const float offset_y)
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

void Map::renderTile(const int gid, const int world_x, const int world_y, const float offset_x, const float offset_y)
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

void Map::renderWalkabilityDebug(const float offset_x, const float offset_y)
{
	constexpr float half_w = TILE_WIDTH / 2.0f;
	constexpr float half_h = TILE_HEIGHT / 2.0f;

	for (size_t y = 0; y < _walkability_grid.size(); ++y) 
	{
		for (size_t x = 0; x < _walkability_grid[y].size(); ++x) 
		{
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

std::shared_ptr<Texture2D> Map::getTextureForGID(const int gid) const
{
	const auto it = _tile_textures.find(gid);
	return (it != _tile_textures.end()) ? it->second : nullptr;
}

bool Map::getWalkableForGID(const int gid) const
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

Vector2 Map::worldToIso(const int world_x, const int world_y, const float offset_x, const float offset_y) const
{
	return {
		(world_x - world_y) * (TILE_WIDTH / 2.0f) + offset_x,
		(world_x + world_y) * (TILE_HEIGHT / 2.0f) + offset_y
	};
}


	// =============================================================================
	// Pathfinding
	// =============================================================================

	constexpr float K_DIAGONAL_COST = 1.414f;  // sqrt(2)

	namespace {
		/**
		 * @brief Calculates octile distance heuristic for A*.
		 * More accurate than Manhattan for 8-directional movement.
		 */
		float octileHeuristic(int from_x, int from_y, int to_x, int to_y)
		{
			const float dx = static_cast<float>(std::abs(to_x - from_x));
			const float dy = static_cast<float>(std::abs(to_y - from_y));
			return (dx + dy) + (K_DIAGONAL_COST - 2.0f) * std::min(dx, dy);
		}

		/**
		 * @brief 8-directional neighbor offsets.
		 * Order: N, NE, E, SE, S, SW, W, NW
		 */
		constexpr int NEIGHBOR_DX[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
		constexpr int NEIGHBOR_DY[] = { -1, -1, 0, 1, 1, 1, 0, -1 };
		constexpr float NEIGHBOR_COST[] = { 1.0f, K_DIAGONAL_COST, 1.0f, K_DIAGONAL_COST, 1.0f, K_DIAGONAL_COST, 1.0f, K_DIAGONAL_COST };
		constexpr int MAX_PATHFINDING_ITERATIONS = 5000;
	} // anonymous namespace

	bool Map::isGridWalkable(const int grid_x, const int grid_y) const
	{
		if (grid_y < 0 || grid_y >= static_cast<int>(_walkability_grid.size()))
			return false;
		if (grid_x < 0 || grid_x >= static_cast<int>(_walkability_grid[grid_y].size()))
			return false;

		return _walkability_grid[grid_y][grid_x];
	}

	Vector2 Map::gridToWorld(const int grid_x, const int grid_y) const
	{
		// Center of the tile
		return Vector2 {
			static_cast<float>(grid_x + _offset_x) + 0.5f,
			static_cast<float>(grid_y + _offset_y) + 0.5f
		};
	}

	std::vector<Vector2> Map::findPath(const Vector2 start_world, const Vector2 end_world) const
	{
		// Convert world to grid coordinates
		const int start_x = static_cast<int>(std::floor(start_world.x)) - _offset_x;
		const int start_y = static_cast<int>(std::floor(start_world.y)) - _offset_y;
		const int end_x = static_cast<int>(std::floor(end_world.x)) - _offset_x;
		const int end_y = static_cast<int>(std::floor(end_world.y)) - _offset_y;

		// Early out checks
		if (!isGridWalkable(start_x, start_y) || !isGridWalkable(end_x, end_y))
			return {};
		if (start_x == end_x && start_y == end_y)
			return {};

		// A* setup
		auto nodeCmp = [](const std::shared_ptr<Node>& a, const std::shared_ptr<Node>& b) {
			return a->f_cost() > b->f_cost();
		};
		std::priority_queue<std::shared_ptr<Node>, std::vector<std::shared_ptr<Node>>, decltype(nodeCmp)> open_set(nodeCmp);
		std::vector<std::vector<bool>> closed_set(_walkability_grid.size(), std::vector<bool>(_walkability_grid[0].size(), false));

		// Initialize start node
		const auto start_node = std::make_shared<Node>();
		start_node->x = start_x;
		start_node->y = start_y;
		start_node->g_cost = 0;
		start_node->h_cost = octileHeuristic(start_x, start_y, end_x, end_y);
		open_set.push(start_node);

		// A* main loop
		int iterations = 0;
		while (!open_set.empty()) 
		{
			const auto current = open_set.top();
			open_set.pop();

			if (iterations++ > MAX_PATHFINDING_ITERATIONS) 
			{
				Logger::errorLog("Pathfinding took too long, aborting.");
				return {};
			}

			// Goal reached - reconstruct path
			if (current->x == end_x && current->y == end_y) 
			{
				std::vector<Vector2> path;
				for (auto node = current; node != nullptr; node = node->parent)
					path.push_back(gridToWorld(node->x, node->y));
				std::reverse(path.begin(), path.end());
				path = simplifyPath(path);
				if (!path.empty()) path.erase(path.begin());
				return path;
			}

			if (closed_set[current->y][current->x])
				continue;
			closed_set[current->y][current->x] = true;

			// Process neighbors
			for (int i = 0; i < 8; ++i) 
			{
				const int nx = current->x + NEIGHBOR_DX[i];
				const int ny = current->y + NEIGHBOR_DY[i];

				if (!isGridWalkable(nx, ny) || closed_set[ny][nx])
					continue;

				// Prevent diagonal corner cutting
				if (i % 2 != 0) {
					const int cx1 = current->x + NEIGHBOR_DX[(i + 7) % 8];
					const int cy1 = current->y + NEIGHBOR_DY[(i + 7) % 8];
					const int cx2 = current->x + NEIGHBOR_DX[(i + 1) % 8];
					const int cy2 = current->y + NEIGHBOR_DY[(i + 1) % 8];
					if (!isGridWalkable(cx1, cy1) || !isGridWalkable(cx2, cy2))
						continue;
				}

				auto neighbor = std::make_shared<Node>();
				neighbor->x = nx;
				neighbor->y = ny;
				neighbor->g_cost = current->g_cost + NEIGHBOR_COST[i];
				neighbor->h_cost = octileHeuristic(nx, ny, end_x, end_y);
				neighbor->parent = current;
				open_set.push(neighbor);
			}
		}

		return {}; // No path found
	}

	void Map::renderPathDebug(const std::vector<Vector2>& path, const float offset_x, const float offset_y)
	{
		if (path.empty()) return;

		for (size_t i = 0; i < path.size(); ++i) {
			Vector2 p = worldToIso(static_cast<int>(path[i].x), static_cast<int>(path[i].y), offset_x, offset_y);
			// Center it visually on tile (approx)
			// Adjusting to center of diamonds
			p.x += 0.0f; 
			p.y += TILE_HEIGHT / 2.0f;
			
			DrawCircleV(p, 5.0f, BLUE);

			if (i > 0) {
				Vector2 prev = worldToIso(static_cast<int>(path[i-1].x), static_cast<int>(path[i-1].y), offset_x, offset_y);
				prev.y += TILE_HEIGHT / 2.0f;
				DrawLineEx(prev, p, 2.0f, BLUE);
			}
		}
	}

	// =============================================================================
	// Path Smoothing
	// =============================================================================

	bool Map::hasLineOfSight(const Vector2 start, const Vector2 end) const
	{
		// Bresenham's Line Algorithm tailored for grid checking
		// Check every tile the line intersects
		
		const float x0 = start.x - _offset_x;
		const float y0 = start.y - _offset_y;
		const float x1 = end.x - _offset_x;
		const float y1 = end.y - _offset_y;

		// ray casting on grid (DDA or similar).
		
		int x_current = static_cast<int>(std::floor(x0));
		int y_current = static_cast<int>(std::floor(y0));
		const int x_end = static_cast<int>(std::floor(x1));
		const int y_end = static_cast<int>(std::floor(y1));

		const int dx = std::abs(x_end - x_current);
		const int dy = std::abs(y_end - y_current);
		
		const int sx = (x_current < x_end) ? 1 : -1;
		const int sy = (y_current < y_end) ? 1 : -1;
		
		int err = dx - dy;

		// Max iterations to prevent freezes
		int ops = 0;
		constexpr int MAX_OPS = 1000;

		while (true) {
			if (!isGridWalkable(x_current, y_current))
				return false;
				
			if (x_current == x_end && y_current == y_end)
				break;
			
			if (ops++ > MAX_OPS) return false;

			const int e2 = 2 * err;
			
			// Diagonal movement handling - check for "cutting corners"
			// If we move diagonal, we must check adjacent orthogonal cells
			
			int next_x = x_current;
			int next_y = y_current;
			bool move_x = false;
			bool move_y = false;
			
			if (e2 > -dy) 
			{
				err -= dy;
				next_x += sx;
				move_x = true;
			}
			if (e2 < dx) 
			{
				err += dx;
				next_y += sy;
				move_y = true;
			}
			
			// If moving diagonally (both changed)
			if (move_x && move_y) {
				if (!isGridWalkable(x_current + sx, y_current) || !isGridWalkable(x_current, y_current + sy))
					return false;
			}

			x_current = next_x;
			y_current = next_y;
		}

		return true;
	}

	std::vector<Vector2> Map::simplifyPath(const std::vector<Vector2>& path) const
	{
		if (path.size() <= 2)
			return path;

		std::vector<Vector2> smoothed;
		smoothed.push_back(path[0]);
		
		int current_idx = 0;
		
		while (current_idx < path.size() - 1) {
			// Look ahead as far as possible
			int valid_next = current_idx + 1;
			
			for (int i = current_idx + 2; i < path.size(); ++i) 
			{
				if (hasLineOfSight(smoothed.back(), path[i])) 
					valid_next = i;
				else 
					break; 
			}
			
			smoothed.push_back(path[valid_next]);
			current_idx = valid_next;
		}
		
		return smoothed;
	}

} // namespace Nawia::Core