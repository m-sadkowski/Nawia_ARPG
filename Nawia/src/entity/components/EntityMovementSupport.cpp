#include "EntityMovementSupport.h"

#include <cmath>

namespace Nawia::Entity {
namespace {
	constexpr float MOVEMENT_TARGET_DISTANCE_SQ = 0.001f;
	constexpr float MOVEMENT_REACHED_DISTANCE = 0.001f;
}

	bool hasMovementTarget(const Vector2 position, const Vector2 target)
	{
		const float dx = target.x - position.x;
		const float dy = target.y - position.y;
		return dx * dx + dy * dy > MOVEMENT_TARGET_DISTANCE_SQ;
	}

	MovementAdvanceResult advanceMovementTowards(
		const Vector2 position,
		const Vector2 target,
		const float move_distance)
	{
		const float dx = target.x - position.x;
		const float dy = target.y - position.y;
		const float distance = std::sqrt(dx * dx + dy * dy);

		if (distance <= MOVEMENT_REACHED_DISTANCE)
			return {target, false, false};

		if (move_distance >= distance)
			return {target, false, true};

		return {
			{
				position.x + (dx / distance) * move_distance,
				position.y + (dy / distance) * move_distance
			},
			true,
			true
		};
	}

}
