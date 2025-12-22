#pragma once
#include "raylib.h"
#include <memory>

namespace Nawia::Core {

	struct Tile {
		Tile(const int id, const std::shared_ptr<Texture2D>& tex) : type_id(id), texture(tex), is_walkable(true) {}
		Tile() : type_id(-1), is_walkable(false) {}

		void setIsWalkable(const bool walkable) { is_walkable = walkable; }

		int type_id;
		std::shared_ptr<Texture2D> texture;
		bool is_walkable;
	};

} // namespace Nawia::Core