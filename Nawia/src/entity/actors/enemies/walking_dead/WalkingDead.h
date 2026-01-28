#pragma once
#include "EnemyInterface.h"

#include <Map.h>

namespace Nawia::Entity {

	/**
	 * @class WalkingDead
	 * @brief Slow undead enemy with reactive hit animations and variable speed.
	 * 
	 * Walking Dead are undead creatures that chase the player when detected.
	 * They walk slowly at distance but run when close. When hit, they play
	 * a stagger animation before resuming their previous behavior.
	 * 
	 * ## States
	 * - Idle: Standing still, watching for player in vision range
	 * - Chasing: Moving toward player (walks far, runs when close)
	 * - Attacking: Melee bite attack when in range
	 * - GettingHit: Stagger animation when damaged (resets on each hit)
	 * - Screaming: Frustration animation when player escapes vision
	 * - Dying: Death animation, then marked as dead
	 * 
	 * ## Behavior
	 * - Starts idle, activates when player enters VISION_RANGE
	 * - Switches to running when player is within CLOSE_RANGE
	 * - Attacks when within ATTACK_RANGE with cooldown
	 * - Taking damage interrupts current action and plays get_hit animation
	 * - Screams when target is lost or escapes vision range
	 */
	class WalkingDead : public EnemyInterface {
	public:
		WalkingDead(float x, float y, Core::Map* map);

		void update(float dt) override;
		void takeDamage(int dmg) override;
		

	private:
		enum class State { Idle, Chasing, Attacking, Screaming, GettingHit, Dying };
		State _state = State::Idle;
		State _state_before_hit = State::Idle;  // State to return to after get_hit animation


		
		// Combat stats
		static constexpr float VISION_RANGE = 6.0f;
		static constexpr float CLOSE_RANGE = VISION_RANGE / 2;    // Distance at which zombie starts running
		static constexpr float ATTACK_RANGE = 1.0f;
		static constexpr float SPEED = 1.0f;
		static constexpr float RUN_SPEED = 3.0f;      // Fast speed when close to player
		static constexpr int ATTACK_DAMAGE = 25;
		static constexpr float ATTACK_COOLDOWN = 2.0f;
		
		float _attack_cooldown_timer = 0.0f;
		bool _is_running = false;  // Track if currently running
		
		// Pathfinding
		static constexpr float DIRECT_MOVE_DISTANCE = 2.0f;

		// State handlers
		void handleIdleState(float dt);
		void handleChasingState(float dt);
		void handleAttackingState(float dt);
		void handleScreamingState(float dt);
		void handleGettingHitState(float dt);
		void handleDyingState(float dt);
		

	};

} // namespace Nawia::Entity
