#include "AbilityEffect.h"

namespace Nawia::Entity {

	AbilityEffect::AbilityEffect(const float x, const float y, const std::shared_ptr<SDL_Texture> &tex, const float duration, const int damage)
	    : Entity(x, y, tex, 1), _duration(duration), _damage(damage), _timer(0.0f) {}

	void AbilityEffect::update(const float dt) { _timer += dt; }

	bool AbilityEffect::isExpired() const { return _timer >= _duration; }

	int AbilityEffect::getDamage() const { return _damage; }

	// base implementation does no collision
	bool AbilityEffect::checkCollision(const std::shared_ptr<Entity>& target) const { return false; }

	void AbilityEffect::onCollision(const std::shared_ptr<Entity>& target)
	{
	  // base behavior: track the hit so we don't hit again immediately if logic requires
	  addHit(target);
	}

	bool AbilityEffect::hasHit(const std::shared_ptr<Entity>& target) const
	{
		for (const auto& weak_ref : _hit_entities) {
			if (auto shared_ref = weak_ref.lock()) {
				if (shared_ref == target) {
					return true;
				}
			}
		}
	}

	void AbilityEffect::addHit(const std::shared_ptr<Entity>& target)
	{
		_hit_entities.emplace_back(target);
	}

} // namespace Nawia::Entity