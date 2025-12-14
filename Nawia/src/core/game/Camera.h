#pragma once
#include "Constants.h"
#include "Entity.h"

namespace Nawia::Core
{
	struct Camera
	{
		float _x = 0.0f;
		float _y = 0.0f;

		void follow(Entity::Entity* target)
		{
			if (!target) return;

			float _world_x = target->getX();
			float _world_y = target->getY();

			float _player_iso_x = (_world_x - _world_y) * (TILE_WIDTH / 2.0f);
			float _player_iso_y = (_world_x + _world_y) * (TILE_HEIGHT / 2.0f);

			_x = (WINDOW_WIDTH / 2.0f) - _player_iso_x;
			_y = (WINDOW_HEIGHT / 2.0f) - _player_iso_y;
		}
	};
}