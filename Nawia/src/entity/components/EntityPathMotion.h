#pragma once

#include <raylib.h>

#include <cstddef>
#include <vector>

namespace Nawia::Core {
	class Map;
}

namespace Nawia::Entity {
	class Entity;
}

namespace Nawia::Entity::PathMotion {

	constexpr float DEFAULT_START_TRIM_DISTANCE_SQ = 0.1f;

	std::size_t buildPathToPoint(
		Entity& entity,
		Core::Map* map,
		Vector2 target,
		std::vector<Vector2>& path);

	std::size_t buildPathToEntity(
		Entity& entity,
		Core::Map* map,
		const Entity& target,
		std::vector<Vector2>& path);

	void trimPathStart(
		const Entity& entity,
		std::vector<Vector2>& path,
		float reached_distance_sq = DEFAULT_START_TRIM_DISTANCE_SQ);

	void updatePathMovement(Entity& entity, float delta_time, std::vector<Vector2>& path);
	void stopPathMovement(Entity& entity, std::vector<Vector2>& path);

} // namespace Nawia::Entity::PathMotion
