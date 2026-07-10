#pragma once

#include <raylib.h>

#include <memory>

namespace Nawia::Entity {
	class Entity;
}

namespace Nawia::Entity::TargetingSupport {

	[[nodiscard]] bool hasLiveTarget(const std::weak_ptr<Entity>& target);
	[[nodiscard]] Vector2 targetPositionOrSelf(const Entity& owner, const std::weak_ptr<Entity>& target);
	[[nodiscard]] float distanceToTarget(const Entity& owner, const std::weak_ptr<Entity>& target);

}
