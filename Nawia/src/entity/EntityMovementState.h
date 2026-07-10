#pragma once

#include <raylib.h>

namespace Nawia::Entity {

	struct EntityMovementState {
		Vector2 position = {0.0f, 0.0f};
		float altitude = 0.0f;
		Vector2 velocity = {0.0f, 0.0f};
		float scale = 1.0f;
		float rotation = 0.0f;
		bool is_moving = false;
		float movement_speed = 2.0f;
		Vector2 target = {0.0f, 0.0f};
		float speed_multiplier = 1.0f;
		float damage_multiplier = 1.0f;
		float path_recalc_timer = 0.0f;
	};

} // namespace Nawia::Entity
