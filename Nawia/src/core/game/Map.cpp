#include "Map.h"
#include "Logger.h"

#include <fstream>
#include <iostream>
#include <limits>

using json = nlohmann::json;

namespace Nawia::Core {

	Map::Map(ResourceManager &resource_manager) : _resource_manager(resource_manager) 
	{
		if (!loadTiles())
			Logger::errorLog("Map - Couldn't load tiles from file.");
	}

	/*
	        LOADS ALL TILES INTO _tiles
	        Always save tile file into /assets/maps/tiles.json
	*/
	bool Map::loadTiles() 
	{
		std::ifstream file("../assets/maps/tiles.json");
		if (!file.is_open()) {
			Logger::errorLog("Load Tiles - Couldn't open file /assets/maps/tiles.json");
			return false;
		}

		json tiles_data;
		try {
			file >> tiles_data;
		} 
		catch (const json::parse_error &e) {
			(void)e;
			Logger::errorLog("Load Tiles - Couldn't parse json file");
			return false;
		}

		// load tiles from json file
		if (tiles_data.contains("tiles")) 
		{
			for (const auto& tile_def : tiles_data["tiles"]) 
			{
				// filename
				std::string raw_path = tile_def["image"];
				std::string filename = std::filesystem::path(raw_path).filename().string();

				/*
				* PROPERTIES
				*
				* Addding a new property:
				* - create a temporary variable
				* - in for loop get the desired variable
				* - add it to _tiles.emplace_back
				* 
				*/
				bool walkable = true;
				if (tile_def.contains("properties"))
				for (const auto &prop : tile_def["properties"])
					if (prop["name"] == "isWalkable")
						walkable = prop["value"];
				/*
				* END OF PROPERTIES
				*/

				_tiles.emplace_back(0, _resource_manager.getTexture("../assets/textures/" + filename));
				_tiles.back().setIsWalkable(walkable);
			}
		}
		return true;
	}

	void Map::loadMap(const std::string &filename) 
	{
		std::ifstream file("../assets/maps/" + filename);
		if (!file.is_open()) {
			Logger::errorLog("Load Map - Couldn't open file " + filename);
			return;
		}

		json map_data;
		try 
		{
			file >> map_data;
		} 
		catch (json::parse_error &e) 
		{
			(void)e;
			Logger::errorLog("Load Map - Couldn't parse file " + filename);
		}

		int min_x = std::numeric_limits<int>::max();
		int min_y = std::numeric_limits<int>::max();
		int max_x = std::numeric_limits<int>::lowest();
		int max_y = std::numeric_limits<int>::lowest();

		auto& layers = map_data["layers"];
		for (const auto &layer : layers) 
		{
			if (layer["type"] == "tilelayer" && layer.contains("chunks")) 
			{
				for (const auto& chunk : layer["chunks"]) 
				{
					const int cx = chunk["x"];
					const int cy = chunk["y"];
					const int cw = chunk["width"];
					const int ch = chunk["height"];

					if (cx < min_x)
						min_x = cx;
					if (cy < min_y)
						min_y = cy;
					if (cx + cw > max_x)
						max_x = cx + cw;
					if (cy + ch > max_y)
						max_y = cy + ch;
				}
			}
		}

		// if map empty, return
		if (min_x == std::numeric_limits<int>::max())
			return;

		int total_width = max_x - min_x;
		int total_height = max_y - min_y;

		// resize grid
		_grid.clear();
		_grid.resize(total_height, std::vector<Tile>(total_width, Tile()));

		const int first_gid = map_data["tilesets"][0]["firstgid"];

		for (const auto& layer : layers) 
		{
			if (layer["type"] == "tilelayer" && layer.contains("chunks")) 
			{
				for (const auto& chunk : layer["chunks"]) 
				{
					const int chunk_x = chunk["x"];
					const int chunk_y = chunk["y"];
					const int width = chunk["width"];
					const int height = chunk["height"];
				    const auto& data = chunk["data"];

				    int data_index = 0;
				    for (int y = 0; y < height; ++y) 
					{
						for (int x = 0; x < width; ++x) 
						{
							const int grid = data[data_index];
							data_index++;

							if (grid > 0) 
							{
								const int tile_id = grid - first_gid;

								if (tile_id >= 0 && tile_id < _tiles.size()) 
								{
									const int grid_x = (chunk_x + x) - min_x;
									const int _grid_y = (chunk_y + y) - min_y;

									_grid[_grid_y][grid_x] = _tiles[tile_id];
								}
							}
						}
					}
				}
			} 
			else if (layer["type"] == "objectgroup") 
			{
				if (layer.contains("objects")) 
				{
					for (const auto &obj : layer["objects"]) 
					{
						if (obj["name"] == "playerspawn") 
						{
							float raw_x = obj["x"];
							float raw_y = obj["y"];

							if (map_data.contains("tileheight")) 
							{
								float th = map_data["tileheight"];

								float grid_x = (raw_x / th);
								float grid_y = (raw_y / th);

								grid_x -= min_x;
								grid_y -= min_y;

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


	void Map::loadTestMap() {
		_grid.clear();
		constexpr int size = 10;
		for (int y = 0; y < size; ++y) 
		{
			std::vector<Tile> row;
			for (int x = 0; x < size; ++x) 
			{
				Tile t = _tiles[0];
				row.push_back(t);
			}
			_grid.push_back(row);
		}
	}

	void Map::render(const float offsetX, const float offset_y) 
	{
		/*
		      Render map on OFFSET (x+400, y)

		      Using currently loaded map in _grid
		      To load a map use Map::loadMap(string filename) function
		      and pass JUST filename into it, for example
		      loadMap("CITY1.map") - loads a map from "../assets/maps/CITY1.map"
		*/
		for (int y = 0; y < _grid.size(); y++) 
		{
			for (int x = 0; x < _grid[y].size(); x++) 
			{
				Tile& tile = _grid[y][x];
				if (tile.texture) 
				{
					const float iso_x = (x - y) * (TILE_WIDTH / 2.0f) + offsetX;
					const float iso_y = (x + y) * (TILE_HEIGHT / 2.0f) + offset_y;
					const Texture2D* tex = tile.texture.get();
					const Rectangle source = {0.0f, 0.0f, (float)tex->width, (float)tex->height};
					const Rectangle dest = {iso_x, iso_y, (float)TILE_WIDTH, (float)TILE_HEIGHT};
					constexpr Vector2 origin = {0.0f, 0.0f};
					DrawTexturePro(*tex, source, dest, origin, 0.0f, WHITE);
				}
			}
		}
	}

	bool Map::isWalkable(const int world_x, const int world_y) const 
	{
		if (world_y < 0 || world_y >= _grid.size())
			return false;
		if (world_x < 0 || world_x >= _grid[world_y].size())
			return false;

		// Logger::debugLog("Grid int: " + std::to_string(world_x) + ", " + std::to_string(world_y));
		// Logger::debugLog("isWalkable: " + std::to_string(_grid[world_y][world_x].is_walkable));

		return _grid[world_y][world_x].is_walkable;
	}

} // namespace Nawia::Core