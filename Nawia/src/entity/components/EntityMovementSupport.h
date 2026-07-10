#pragma once

#include <raylib.h>

namespace Nawia::Entity {

	struct MovementAdvanceResult {
		Vector2 position = {0.0f, 0.0f};
		bool moving = false;
		bool should_face_target = false;
	};

	[[nodiscard]] bool hasMovementTarget(Vector2 position, Vector2 target);
	[[nodiscard]] MovementAdvanceResult advanceMovementTowards(Vector2 position, Vector2 target, float move_distance);

}
