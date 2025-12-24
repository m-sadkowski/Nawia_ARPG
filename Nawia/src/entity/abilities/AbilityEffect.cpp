#include "AbilityEffect.h"
#include "Collider.h"

#include <algorithm>

namespace Nawia::Entity {

	AbilityEffect::AbilityEffect(const std::string& name, const float x, const float y, const std::shared_ptr<Texture2D>& tex, const AbilityStats& stats) 
		: Entity(name, x, y, tex, 1), _stats(stats), _timer(0.0f) {}

	void AbilityEffect::update(const float dt)
	{
		_timer += dt;
	}

	bool AbilityEffect::isExpired() const
	{
		return _timer >= _stats.duration;
	}

	int AbilityEffect::getDamage() const
	{
		return _stats.damage;
	}

	bool AbilityEffect::checkCollision(const std::shared_ptr<Entity>& target) const
	{
		if (!target) return false;
		Collider* myCollider = getCollider();
		Collider* targetCollider = target->getCollider();

		if (myCollider && targetCollider)
		{
			return myCollider->checkCollision(targetCollider);
		}
		
		// Fallback or just return false if no collider
		return false;
	}

	void AbilityEffect::onCollision(const std::shared_ptr<Entity>& target) 
	{
		addHit(target);
	}

	bool AbilityEffect::hasHit(const std::shared_ptr<Entity>& target) const 
	{
		// checks if target exists in the hit list, safely handling expired pointers
		return std::any_of(_hit_entities.begin(), _hit_entities.end(), [&target](const auto& weak_ref) 
			{
				return weak_ref.lock() == target;
			}
		);
	}

	void AbilityEffect::addHit(const std::shared_ptr<Entity>& target) {
		_hit_entities.emplace_back(target);
	}

} // namespace Nawia::Entity