#pragma once
#include "Constants.h"

#include <Entity.h>

namespace Nawia::Core
{

	struct Camera
	{
		float x = 0.0f;
		float y = 0.0f;

		void follow(const Entity::Entity* target)
		{
			if (!target) return;

			const float world_x = target->getX();
			const float world_y = target->getY();

			const float player_iso_x = (world_x - world_y) * (TILE_WIDTH / 2.0f);
			const float player_iso_y = (world_x + world_y) * (TILE_HEIGHT / 2.0f);

			x = (WINDOW_WIDTH / 2.0f) - player_iso_x;
			y = (WINDOW_HEIGHT / 2.0f) - player_iso_y;
		}
	};

} // namespace Nawia::Core