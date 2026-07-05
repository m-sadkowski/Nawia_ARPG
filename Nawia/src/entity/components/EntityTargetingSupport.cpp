#include "EntityTargetingSupport.h"

#include <Entity.h>

#include <raymath.h>

#include <limits>

namespace Nawia::Entity::TargetingSupport {

	bool hasLiveTarget(const std::weak_ptr<Entity>& target)
	{
		const auto live_target = target.lock();
		return live_target && !live_target->isDead();
	}

	Vector2 targetPositionOrSelf(const Entity& owner, const std::weak_ptr<Entity>& target)
	{
		const auto live_target = target.lock();
		if (!live_target)
			return owner.getCenter();

		return live_target->getCenter();
	}

	float distanceToTarget(const Entity& owner, const std::weak_ptr<Entity>& target)
	{
		const auto live_target = target.lock();
		if (!live_target)
			return std::numeric_limits<float>::max();

		return Vector2Distance(owner.getCenter(), live_target->getCenter());
	}

}
