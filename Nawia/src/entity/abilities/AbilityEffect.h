#pragma once
#include "Entity.h"
#include "AbilityStats.h"

#include <vector>

namespace Nawia::Entity {

	class AbilityEffect : public Entity {
	public:
		AbilityEffect(float x, float y, const std::shared_ptr<Texture2D>& tex, const AbilityStats& stats);

		void update(float dt) override;
		[[nodiscard]] bool isExpired() const;
		[[nodiscard]] int getDamage() const;
		[[nodiscard]] const AbilityStats& getStats() const { return _stats; }

		// collision handling
		[[nodiscard]] virtual bool checkCollision(const std::shared_ptr<Entity>& target) const;
		virtual void onCollision(const std::shared_ptr<Entity>& target);
		[[nodiscard]] bool hasHit(const std::shared_ptr<Entity>& target) const;
		void addHit(const std::shared_ptr<Entity>& target);

	protected:
		AbilityStats _stats;
		float _timer;
		std::vector<std::weak_ptr<Entity>> _hit_entities;
	};

} // namespace Nawia::Entity