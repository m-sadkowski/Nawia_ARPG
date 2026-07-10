#pragma once

#include <Entity.h>

#include <algorithm>
#include <raylib.h>

namespace Nawia::Game::AgentSystemMath {

	[[nodiscard]] inline float distanceSquared(const Vector2 first, const Vector2 second) {
		const float dx = second.x - first.x;
		const float dy = second.y - first.y;
		return dx * dx + dy * dy;
	}

	[[nodiscard]] inline float distanceToBoxSquared(const Entity::Entity& entity, const Vector2 position) {
		const BoundingBox box = entity.getBoundingBox();
		const float closest_x = std::clamp(position.x, box.min.x, box.max.x);
		const float closest_y = std::clamp(position.y, box.min.z, box.max.z);
		const float dx = position.x - closest_x;
		const float dy = position.y - closest_y;
		return dx * dx + dy * dy;
	}

} // namespace Nawia::Game::AgentSystemMath
