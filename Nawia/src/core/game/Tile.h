#pragma once
#include <SDL3/SDL.h>
#include <memory>

namespace Nawia::Core {

	struct Tile {
		int type_id;
		std::shared_ptr<SDL_Texture> texture;
		bool is_walkable;

		Tile(int id, const std::shared_ptr<SDL_Texture>& tex, const bool walkable = true) : type_id(id), texture(tex), is_walkable(walkable) {}
		Tile() : type_id(-1), is_walkable(false) {}
	};

} // namespace Nawia::Core