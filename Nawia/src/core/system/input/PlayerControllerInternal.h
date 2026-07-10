#pragma once

#include <EnemyInterface.h>
#include <Entity.h>

#include <algorithm>
#include <memory>

namespace Nawia::Core::PlayerControllerDetail {

	inline float horizontalDistanceToBoxSq(const Entity::Entity& entity, const Vector2 position) {
		const BoundingBox box = entity.getBoundingBox();
		const float closest_x = std::clamp(position.x, box.min.x, box.max.x);
		const float closest_y = std::clamp(position.y, box.min.z, box.max.z);
		const float dx = position.x - closest_x;
		const float dy = position.y - closest_y;
		return dx * dx + dy * dy;
	}

	inline bool isValidEnemyTarget(const std::shared_ptr<Entity::EnemyInterface>& enemy) {
		return enemy && !enemy->isDead() && enemy->getFaction() != Entity::Faction::None;
	}

} // namespace Nawia::Core::PlayerControllerDetail
