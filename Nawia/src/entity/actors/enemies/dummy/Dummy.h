#pragma once
#include "EnemyInterface.h"

#include <Map.h>

namespace Nawia::Entity {

	class Dummy : public EnemyInterface {
	public:
		Dummy(float x, float y, const std::shared_ptr<Texture2D>& tex, int max_hp, Core::Map* map);

		void update(float dt) override;
		void setTarget(const std::shared_ptr<Entity>& target) { _target = target; }
		void takeDamage(int dmg) override;

	private:
		float _target_x, _target_y;
		float _stay_timer;
		std::shared_ptr<Entity> _target;
		float _fireball_cooldown_timer;
		bool _is_dying = false;
		bool _is_casting = false;

		void pickNewTarget();
		
		void handleDyingState(float dt);
		void handleCastingState(float dt);
		void handleActiveState(float dt);
	};

} // namespace Nawia::Entity
