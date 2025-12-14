#pragma once
#include "ResourceManager.h"
#include "Tile.h"

#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <json.hpp>

namespace Nawia::Core {

	constexpr int TILE_WIDTH = 128;
	constexpr int TILE_HEIGHT = 64;

	constexpr float MAP_OFFSET_X = 500.0f;
	constexpr float MAP_OFFSET_Y = 0.0f;

	class Map {
	public:
		Map(SDL_Renderer* renderer, ResourceManager& resource_manager);

		bool loadTiles();
		void loadMap(const std::string& filename);
		void loadTestMap();
		void render();

		[[nodiscard]] bool isWalkable(int grid_x, int grid_y) const;

	private:
		SDL_Renderer* _renderer;
		ResourceManager& _resource_manager;

		std::vector<Tile> _tiles;
		std::vector<std::vector<Tile>> _grid;
	};

} // namespace Nawia::Core