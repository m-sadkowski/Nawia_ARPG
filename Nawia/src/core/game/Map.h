#pragma once
#include "ResourceManager.h"
#include "Tile.h"
#include "Constants.h"

#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <json.hpp>

namespace Nawia::Core 
{

	class Map {
	public:
		Map(SDL_Renderer* renderer, ResourceManager& resource_manager);

		bool loadTiles();
		void loadMap(const std::string& filename);
		void loadTestMap();
		void render(float offsetX, float offsetY);

		[[nodiscard]] bool isWalkable(int world_x, int world_y) const;

	private:
		SDL_Renderer* _renderer;
		ResourceManager& _resource_manager;

		int _offset_x, _offset_y;

		std::vector<Tile> _tiles;
		std::vector<std::vector<Tile>> _grid;
	};

} // namespace Nawia::Core