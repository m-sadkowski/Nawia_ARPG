#pragma once

#include "EnemyInterface.h"

#include <Map.h>

namespace Nawia::Entity {

	class Dummy : public EnemyInterface {
	public:
		Dummy(float x, float y, const std::shared_ptr<Texture2D>& tex, int max_hp, Core::Map* map);

		void update(float dt) override;
		void takeDamage(int dmg) override;

	private:
		float _stay_timer;
		float _fireball_cooldown_timer;
		bool _is_casting = false;

		void pickNewTarget();
		
		void handleCastingState(float dt);
		void handleActiveState(float dt);
	};

} // namespace Nawia::Entity
