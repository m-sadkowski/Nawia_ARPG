#include "AbilityEffect.h"

#include <Collider.h>

#include <algorithm>

namespace Nawia::Entity {

	AbilityEffect::AbilityEffect(const std::string& name,
								 const float x,
								 const float y,
								 const std::shared_ptr<Texture2D>& tex,
								 const AbilityStats& stats)
		: Entity(name, x, y, tex, 1),
		  _stats(stats) {
		setType(EntityType::Projectile);
	}

	void AbilityEffect::update(const float dt) {
		_timer += dt;
	}

	bool AbilityEffect::isExpired() const {
		return _timer >= _stats.duration;
	}

	bool AbilityEffect::canHitTarget(const std::shared_ptr<Entity>& target) const {
		return target && target.get() != this && !target->isDead() && !target->isDying() && !target->isDormant() &&
			   !hasHit(target);
	}

	bool AbilityEffect::checkCollision(const std::shared_ptr<Entity>& target) const {
		if (!canHitTarget(target))
			return false;

		const Collider* effect_collider = getCollider();
		if (!effect_collider)
			return false;

		return effect_collider->checkCollision(target->getBoundingBox());
	}

	void AbilityEffect::onCollision(const std::shared_ptr<Entity>& target) {
		addHit(target);
	}

	bool AbilityEffect::hasHit(const std::shared_ptr<Entity>& target) const {
		if (!target)
			return false;

		return std::any_of(_hit_entities.begin(), _hit_entities.end(), [&target](const auto& weak_ref) {
			return weak_ref.lock() == target;
		});
	}

	void AbilityEffect::addHit(const std::shared_ptr<Entity>& target) {
		if (target)
			_hit_entities.emplace_back(target);
	}

} // namespace Nawia::Entity
