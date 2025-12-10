#include "Map.h"

namespace Nawia::Core {

	void Map::loadMap(const std::string& filename) {
		//std::ifstream file(filename);

		
	}

	// REMOVE - load test map
	void Map::loadTestMap() {
		_grid.clear();
		int size = 10;

		for (int y = 0; y < size; ++y) {
			std::vector<Tile> row;
			for (int x = 0; x < size; ++x) {
				int id = 0;
				if (x == 0 || y == 0 || x == size - 1 || y == size - 1) id = 0;
				else if (x == 5 && y == 5) id = 0;

				row.push_back(Tile(0, _resourceManager.getTexture("../assets/textures/grass.png", _renderer), true));
			}
			_grid.push_back(row);
		}
	}

	void Map::render() {
		/*
			Render map on OFFSET (x+400, y)

			Using currently loaded map in _grid
			To load a map use Map::loadMap(string filename) function
			and pass JUST filename into it, for example
			loadMap("CITY1.map") - loads a map from "../assets/maps/CITY1.map"
		*/
		for (auto y = 0; y < _grid.size(); y++) {
			for (auto x = 0; x < _grid[y].size(); x++) {
				Tile& tile = _grid[y][x];
				

				if (tile._texture) {
					float isoX = (x - y) * (TILE_WIDTH / 2.0f) + 500;
					float isoY = (x + y) * (TILE_HEIGHT / 2.0f);

					SDL_FRect destRect = { isoX, isoY, (float)TILE_WIDTH, (float)TILE_HEIGHT };
					SDL_RenderTexture(_renderer, tile._texture.get(), nullptr, &destRect);
				}
			}
		}
	}

}