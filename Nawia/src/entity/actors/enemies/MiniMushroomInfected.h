#pragma once

#include <SimpleMeleeEnemy.h>

namespace Nawia::Entity {

	class MiniMushroomInfected : public SimpleMeleeEnemy {
	public:
		MiniMushroomInfected();

		void takeDamage(int dmg) override;
		void update(float dt) override;
		void purifyAfterWormDeath();
		[[nodiscard]] bool isPurified() const { return _purified; }

	private:
		void spawnLinkedWorm();
		void freezeOnDeathFrame();
		void updatePurifiedProp(float dt);
		void loadMiniMushroomAnimations();

		bool _corruption_released = false;
		bool _corpse_frozen = false;
		bool _purified = false;
		bool _purifying = false;
		bool _jumping = false;
		float _jump_timer = 2.0f;
	};

} // namespace Nawia::Entity
