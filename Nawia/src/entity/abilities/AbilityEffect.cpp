#include "AbilityEffect.h"

#include <algorithm>

namespace Nawia::Entity {

	AbilityEffect::AbilityEffect(const float x, const float y, const std::shared_ptr<Texture2D>& tex, const float duration, const int damage) 
		: Entity(x, y, tex, 1), _duration(duration), _damage(damage), _timer(0.0f) {}

	void AbilityEffect::update(const float dt)
	{
		_timer += dt;
	}

	bool AbilityEffect::isExpired() const
	{
		return _timer >= _duration;
	}

	int AbilityEffect::getDamage() const
	{
		return _damage;
	}

	bool AbilityEffect::checkCollision(const std::shared_ptr<Entity>& target) const
	{
		return false;
	}

	void AbilityEffect::onCollision(const std::shared_ptr<Entity>& target) 
	{
		addHit(target);
	}

	bool AbilityEffect::hasHit(const std::shared_ptr<Entity>& target) const 
	{
		// checks if target exists in the hit list, safely handling expired pointers
		return std::ranges::any_of(_hit_entities, [&target](const auto& weak_ref) 
			{
				return weak_ref.lock() == target;
			}
		);
	}

	void AbilityEffect::addHit(const std::shared_ptr<Entity>& target) {
		_hit_entities.emplace_back(target);
	}

} // namespace Nawia::Entity