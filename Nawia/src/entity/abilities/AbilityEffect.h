#pragma once
#include "Entity.h"

#include <vector>

namespace Nawia::Entity {

	class AbilityEffect : public Entity {
	public:
		AbilityEffect(float x, float y, const std::shared_ptr<Texture2D>& tex, float duration, int damage);

		void update(float dt) override;
		[[nodiscard]] bool isExpired() const;
		[[nodiscard]] int getDamage() const;

		// collision handling
		[[nodiscard]] virtual bool checkCollision(const std::shared_ptr<Entity>& target) const;
		virtual void onCollision(const std::shared_ptr<Entity>& target);
		[[nodiscard]] bool hasHit(const std::shared_ptr<Entity>& target) const;
		void addHit(const std::shared_ptr<Entity>& target);

	protected:
		float _duration;
		float _timer;
		int _damage;
		std::vector<std::weak_ptr<Entity>> _hit_entities;
	};

} // namespace Nawia::Entity