#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <SDL3/SDL.h>
#include "ResourceManager.h"
#include "Tile.h"
#include <iostream>

namespace Nawia::Core {

	constexpr int TILE_WIDTH = 128;
	constexpr int TILE_HEIGHT = 64;

	class Map {
	public:
		Map(SDL_Renderer* renderer, ResourceManager& resMgr)
			: _renderer(renderer), _resourceManager(resMgr) {}

		void loadMap(const std::string& filename);
		void loadTestMap();
		void render();

		bool isWalkable(int gridX, int gridY) const;

	private:
		SDL_Renderer* _renderer;
		ResourceManager& _resourceManager;

		std::vector<std::vector<Tile>> _grid;
	};

}