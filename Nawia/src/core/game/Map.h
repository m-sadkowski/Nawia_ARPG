#pragma once
#include "Constants.h"
#include "MathUtils.h"
#include "ResourceManager.h"
#include "Tile.h"

#include <raylib.h>
#include <json.hpp>
#include <string>
#include <vector>

namespace Nawia::Core {

	class Map {
	public:
		explicit Map(ResourceManager& resource_manager);

		[[nodiscard]] bool loadTiles();
		void loadMap(const std::string &filename);
		void loadTestMap();
		void render(float offset_x, float offset_y);


		[[nodiscard]] Vector2 getPlayerSpawnPos() { return _player_spawn_pos; }

		[[nodiscard]] bool isWalkable(int world_x, int world_y) const;

	private:
		ResourceManager& _resource_manager;

		int _offset_x, _offset_y;

		std::vector<Tile> _tiles;
		std::vector<std::vector<Tile>> _grid;

		Vector2 _player_spawn_pos;
	};

} // namespace Nawia::Core