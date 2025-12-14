#include "Map.h"

#include "Logger.h"

using json = nlohmann::json;

namespace Nawia::Core {

	Map::Map(SDL_Renderer* renderer, ResourceManager& resource_manager) 
		: _renderer(renderer), _resource_manager(resource_manager) {
	
		if (!loadTiles()) {
			Logger::errorLog("Map - Couldn't load tiles from file.");
		}

	}

	/*
		LOADS ALL TILES INTO _tiles
		Always save tile file into /assets/maps/tiles.json
	*/
	bool Map::loadTiles() {
		std::ifstream file("../assets/maps/tiles.json");
		if (!file.is_open()) {
			Logger::errorLog("Load Tiles - Couldn't open file /assets/maps/tiles.json");
			return false;
		}

		json _tiles_data;
		try {
			file >> _tiles_data;
		}
		catch (const json::parse_error& e) {
			Logger::errorLog("Load Tiles - Couldn't parse json file");
			return false;
		}

		// load tiles from json file
		if (_tiles_data.contains("tiles")) {
			for (const auto& _tile_def : _tiles_data["tiles"]) {
				int _id = _tile_def["id"];

				// filename
				std::string _raw_path = _tile_def["image"];
				std::string _filename = std::filesystem::path(_raw_path).filename().string();

				// properties
				bool _walkable = true;
				if (_tile_def.contains("properties")) {
					for (const auto& prop : _tile_def["properties"]) {
						if (prop["name"] == "isWalkable") {
							_walkable = prop["value"];
						}
					}
				}

				// load into vector
				_tiles.emplace_back(0, _resource_manager.getTexture("../assets/textures/" + _filename, _renderer), _walkable);
			}
		}
		return true;
	}

	void Map::loadMap(const std::string& filename) {
		// std::ifstream file(filename);
	}

	// REMOVE - load test map
	void Map::loadTestMap() {
		_grid.clear();
		constexpr int size = 10;

		for (int y = 0; y < size; ++y) {
			std::vector<Tile> row;
			for (int x = 0; x < size; ++x) {
				Tile t = _tiles[0];
				row.push_back(t);
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
		for (int y = 0; y < _grid.size(); y++) {
			for (int x = 0; x < _grid[y].size(); x++) {
				Tile& tile = _grid[y][x];

				if (tile.texture) {
					const float iso_x = static_cast<float>(x - y) * (TILE_WIDTH / 2.0f) + 500;
					const float iso_y = static_cast<float>(x + y) * (TILE_HEIGHT / 2.0f);

					SDL_FRect dest_rect = { iso_x, iso_y, static_cast<float>(TILE_WIDTH), static_cast<float>(TILE_HEIGHT) };
					SDL_RenderTexture(_renderer, tile.texture.get(), nullptr, &dest_rect);
				}
			}
		}
	}

} // namespace Nawia::Core