#include "AbilityEffect.h"
#include <Collider.h>

#include <algorithm>

namespace Nawia::Entity {

	AbilityEffect::AbilityEffect(const std::string& name, const float x, const float y, const std::shared_ptr<Texture2D>& tex, const AbilityStats& stats) 
		: Entity(name, x, y, tex, 1), _stats(stats), _timer(0.0f) {
		_type = EntityType::Projectile;
	}

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

		if (myCollider)
		{
		// Szybki test wstępny: kolider efektu kontra pudełko ograniczające celu.
			if (myCollider->checkCollision(target->getBoundingBox()))
			{
				// Faza dokładna: test z siatką modelu, podobny do hovera myszą.
				return myCollider->checkMeshCollision(target.get());
			}
		}
		
		// Bez kolidera efekt nie ma geometrii trafienia.
		return false;
	}

	void AbilityEffect::onCollision(const std::shared_ptr<Entity>& target) 
	{
		addHit(target);
	}

	bool AbilityEffect::hasHit(const std::shared_ptr<Entity>& target) const 
	{
		// Sprawdzamy listę trafionych, bezpiecznie obsługując wygasłe weak_ptr.
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
