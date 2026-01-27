#pragma once
#include "Constants.h"
#include "MathUtils.h"
#include "ResourceManager.h"

#include <raylib.h>
#include <json.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Nawia::Core {

	/**
	 * Stores metadata about a single tileset loaded from .tsx file
	 */
	struct TilesetInfo {
		int firstgid = 0;
		int tilecount = 0;
		std::string source;
		std::unordered_map<int, bool> tile_walkable;  // local_id -> isWalkable
	};

	/**
	 * Manages loading and rendering of Tiled maps with multiple layers.
	 * 
	 * Supports:
	 * - 3 visual layers: ground, on_ground, over_ground (rendered in order)
	 * - 1 logical layer: walkability (not rendered, used for collision)
	 * - Multiple tilesets in .tsx format
	 * - Infinite maps with chunks
	 */
	class Map {
	public:
		explicit Map(ResourceManager& resource_manager);

		void loadMap(const std::string& filename);
		void render(float offset_x, float offset_y);

		[[nodiscard]] Vector2 getPlayerSpawnPos() const { return _player_spawn_pos; }
		[[nodiscard]] bool isWalkable(float world_x, float world_y) const;
		
		void setDebugWalkability(bool enabled) { _debug_walkability = enabled; }

	private:
		ResourceManager& _resource_manager;

		// Map offset for infinite maps with negative coordinates
		int _offset_x = 0;
		int _offset_y = 0;

		// Visual layers (store tile GIDs, 0 = empty)
		std::vector<std::vector<int>> _layer_ground;
		std::vector<std::vector<int>> _layer_on_ground;
		std::vector<std::vector<int>> _layer_over_ground;

		// Collision layer
		std::vector<std::vector<bool>> _walkability_grid;

		// Tileset data
		std::vector<TilesetInfo> _tilesets;
		std::unordered_map<int, std::shared_ptr<Texture2D>> _tile_textures;
		std::unordered_map<int, Rectangle> _tile_source_rects;

		// Map metadata
		Vector2 _player_spawn_pos = {0, 0};
		std::string _map_base_path;
		bool _debug_walkability = false;

		// === Loading (JSON) ===
		bool loadTilesets(const nlohmann::json& map_data);
		bool loadTilesetFile(const std::string& tsx_path, TilesetInfo& tileset);
		void loadLayers(const nlohmann::json& layers, const nlohmann::json& map_data);
		void loadTileLayer(const nlohmann::json& layer, std::vector<std::vector<int>>& grid);
		void loadWalkabilityLayer(const nlohmann::json& layer);
		void loadObjectLayer(const nlohmann::json& layer, const nlohmann::json& map_data);
		void calculateMapBounds(const nlohmann::json& layers, int& min_x, int& min_y, int& max_x, int& max_y);
		void initializeGrids(int width, int height);

		// === Rendering ===
		void renderLayer(const std::vector<std::vector<int>>& layer, float offset_x, float offset_y);
		void renderTile(int gid, int world_x, int world_y, float offset_x, float offset_y);
		void renderWalkabilityDebug(float offset_x, float offset_y);

		// === Utilities ===
		[[nodiscard]] std::shared_ptr<Texture2D> getTextureForGID(int gid) const;
		[[nodiscard]] bool getWalkableForGID(int gid) const;
		[[nodiscard]] Vector2 worldToIso(int world_x, int world_y, float offset_x, float offset_y) const;
	};

} // namespace Nawia::Core