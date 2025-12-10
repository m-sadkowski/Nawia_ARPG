#pragma once
#include <SDL3/SDL.h>
#include <memory>

namespace Nawia::Core {

	struct Tile {
		std::shared_ptr<SDL_Texture> _texture;

		int _typeID;
		bool _isWalkable;

		Tile(int id, std::shared_ptr<SDL_Texture> tex, bool walkable = true)
			: _typeID(id), _texture(tex), _isWalkable(walkable) {}

		Tile() : _typeID(-1), _isWalkable(false) {}
	};

}