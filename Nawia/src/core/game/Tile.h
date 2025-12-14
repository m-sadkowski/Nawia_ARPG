#pragma once
#include <SDL3/SDL.h>
#include <memory>

namespace Nawia::Core {

	struct Tile {
		int type_id;
		std::shared_ptr<SDL_Texture> texture;

		/*
		 * PROPERTIES
		 */

		bool is_walkable;

		/*
		 * END OF PROPERTIES
		 */

		Tile(int id, const std::shared_ptr<SDL_Texture>& tex) : type_id(id), texture(tex) {}
		Tile() : type_id(-1), is_walkable(false) {}

		/*
		 * PROPERTIES SETTERS
		 */
		void setIsWalkable(bool isWalkable) { is_walkable = true; }
		/*
		 * END OF PROPERTIES SETTERS
		 */
	};

} // namespace Nawia::Core