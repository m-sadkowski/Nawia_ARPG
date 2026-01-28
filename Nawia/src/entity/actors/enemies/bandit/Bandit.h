#pragma once
#include "EnemyInterface.h"

#include <Map.h>

namespace Nawia::Entity {

	/**
	 * @class Bandit
	 * @brief Ranged enemy that throws knives at the player.
	 * 
	 * Bandits are hostile humanoids that keep their distance and throw knives.
	 * They occasionally move to new positions to avoid being cornered.
	 * 
	 * ## States
	 * - Idle: Standing still, watching for player
	 * - Chasing: Moving toward player to get in range
	 * - Casting: Throwing knife animation
	 * - Dying: Death animation
	 * 
	 * ## Behavior
	 * - Activates when player enters VISION_RANGE
	 * - Throws knives when player is in ATTACK_RANGE and ability is ready
	 * - Keeps distance, prefers ranged combat
	 */
	class Bandit : public EnemyInterface {
	public:
		Bandit(float x, float y, Core::Map* map);

		void update(float dt) override;
		void takeDamage(int dmg) override;
		

	private:
		enum class State { Idle, Chasing, Casting, GettingHit, Dying };
		State _state = State::Idle;
		State _state_before_hit = State::Idle;


		
		// Combat stats
		static constexpr float VISION_RANGE = 10.0f;
		static constexpr float ATTACK_RANGE = 8.0f;      // Preferred attack distance
		static constexpr float MIN_DISTANCE = 5.0f;      // Try to keep at least this far
		static constexpr float SPEED = 2.0f;
		static constexpr float KNIFE_COOLDOWN = 3.0f;
		
		// Pathfinding for retreat
		static constexpr float PATH_RECALC_INTERVAL = 0.3f;
		float _knife_cooldown_timer = 0.0f;
		bool _is_retreating = false;
		bool _knife_thrown_this_cast = false;  // tracks if knife was thrown during current cast

		// State handlers
		void handleIdleState(float dt);
		void handleChasingState(float dt);
		void handleCastingState(float dt);
		void handleGettingHitState(float dt);
		void handleDyingState(float dt);
		

	};

} // namespace Nawia::Entity
