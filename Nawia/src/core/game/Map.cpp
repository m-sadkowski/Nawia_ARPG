#include "Map.h"
#include "Logger.h"

#include <iostream>
#include <fstream>
#include <limits>

using json = nlohmann::json;

namespace Nawia::Core
{

	Map::Map(SDL_Renderer* renderer, ResourceManager& resource_manager) : _renderer(renderer), _resource_manager(resource_manager) {
		if (!loadTiles())
			Logger::errorLog("Map - Couldn't load tiles from file.");
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
			SDL_UNUSED(e);
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

				/*
				 * PROPERTIES
				 * 
				 * Addding a new property:
				 * - create a temporary variable
				 * - in for loop get the desired variable
				 * - add it to _tiles.emplace_back
				 */
				bool _walkable = true;
				if (_tile_def.contains("properties")) {
					for (const auto& prop : _tile_def["properties"]) {
						if (prop["name"] == "isWalkable") {
							_walkable = prop["value"];
						}
					}
				}
				/*
				 * END OF PROPERTIES
				 */

				// load into vector
				_tiles.emplace_back(0, _resource_manager.getTexture("../assets/textures/" + _filename, _renderer));

				/*
				 * SETTING PROPERTIES
				 */

				_tiles.back().setIsWalkable(_walkable);

				/*
				 * END OF SETTING PROPERTIES
				 */
			}
		}
		return true;
	}

	void Map::loadMap(const std::string& filename) {
		std::ifstream file("../assets/maps/" + filename);
		if (!file.is_open())
		{
			Logger::errorLog("Load Map - Couldn't open file " + filename);
			return;
		}

		json _map_data;
		try
		{
			file >> _map_data;
		} catch (json::parse_error& e)
		{
			SDL_UNUSED(e);
			Logger::errorLog("Load Map - Couldn't parse file " + filename);
		}

		int _min_x = std::numeric_limits<int>::max();
		int _min_y = std::numeric_limits<int>::max();
		int _max_x = std::numeric_limits<int>::lowest();
		int _max_y = std::numeric_limits<int>::lowest();

		auto& _layers = _map_data["layers"];
		for (const auto& _layer : _layers)
		{
			if (_layer["type"] == "tilelayer" && _layer.contains("chunks"))
			{
				for (const auto& _chunk : _layer["chunks"])
				{
					int _cx = _chunk["x"];
					int _cy = _chunk["y"];
					int _cw = _chunk["width"];
					int _ch = _chunk["height"];

					if (_cx < _min_x) _min_x = _cx;
					if (_cy < _min_y) _min_y = _cy;
					if (_cx + _cw > _max_x) _max_x = _cx + _cw;
					if (_cy + _ch > _max_y) _max_y = _cy + _ch;
				}
			}
		}

		// if map empty, return
		if (_min_x == std::numeric_limits<int>::max()) return;

		int _total_width = _max_x - _min_x;
		int _total_height = _max_y - _min_y;

		// resize grid
		_grid.clear();
		_grid.resize(_total_height, std::vector<Tile>(_total_width, Tile()));

		int _first_gid = _map_data["tilesets"][0]["firstgid"];

		for (const auto& _layer : _layers)
		{
			if (_layer["type"] == "tilelayer" && _layer.contains("chunks"))
			{
				for (const auto& _chunk : _layer["chunks"])
				{
					int _chunk_x = _chunk["x"];
					int _chunk_y = _chunk["y"];
					int _width = _chunk["width"];
					int _height = _chunk["height"];
					const auto& _data = _chunk["data"];

					int _data_index = 0;
					for (int y = 0; y < _height; ++y)
					{
						for (int x = 0; x < _width; ++x)
						{
							int _gid = _data[_data_index];
							_data_index++;

							if (_gid > 0)
							{
								int _tile_id = _gid - _first_gid;

								if (_tile_id >= 0 && _tile_id < _tiles.size())
								{
									int _grid_x = (_chunk_x + x) - _min_x;
									int _grid_y = (_chunk_y + y) - _min_y;

									_grid[_grid_y][_grid_x] = _tiles[_tile_id];
								}
							}
						}
					}
				}
			}
			else if (_layer["type"] == "objectgroup")
			{
				if (_layer.contains("objects"))
				{
					for (const auto& obj : _layer["objects"])
					{
						if (obj["name"] == "playerspawn")
						{
							float raw_x = obj["x"];
							float raw_y = obj["y"];

							if (_map_data.contains("tileheight"))
							{
								float th = _map_data["tileheight"];

								float grid_x = (raw_x / th);
								float grid_y = (raw_y / th);

								grid_x -= _min_x;
								grid_y -= _min_y;

								_player_spawn_pos.setX(grid_x);
								_player_spawn_pos.setY(grid_y);
								Logger::debugLog("Map - Loaded player spawn point");
							}
							
						}
					}
				}
			}
		}
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

	void Map::render(float offsetX, float offset_y) {
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
					const float iso_x = static_cast<float>(x - y) * (TILE_WIDTH / 2.0f) + offsetX;
					const float iso_y = static_cast<float>(x + y) * (TILE_HEIGHT / 2.0f) + offset_y;

					SDL_FRect dest_rect = { iso_x, iso_y, static_cast<float>(TILE_WIDTH), static_cast<float>(TILE_HEIGHT) };
					SDL_RenderTexture(_renderer, tile.texture.get(), nullptr, &dest_rect);
				}
			}
		}
	}

	bool Map::isWalkable(int world_x, int world_y) const
	{

		if (world_y < 0 || world_y >= _grid.size()) return false;
		if (world_x < 0 || world_x >= _grid[world_y].size()) return false;

		Logger::debugLog("Grid int: " + std::to_string(world_x) + ", " + std::to_string(world_y));
		Logger::debugLog("isWalkable: " + std::to_string(_grid[world_y][world_x].is_walkable));

		return _grid[world_y][world_x].is_walkable;
	}

} // namespace Nawia::Core