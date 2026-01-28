#pragma once
#include "EnemyInterface.h"

#include <Map.h>

namespace Nawia::Entity {

	/**
	 * @class Devil
	 * @brief Demonic enemy with aggressive dash attack behavior.
	 * 
	 * Devils are demonic creatures that chase the player and perform
	 * quick dash attacks. The dash locks onto the player's position
	 * BEFORE charging, allowing skilled players to dodge.
	 * 
	 * ## States
	 * - Idle: Standing still, watching for player in vision range
	 * - Chasing: Walking toward player
	 * - PreparingDash: Brief pause to lock target position (telegraph)
	 * - Dashing: Fast charge to locked position
	 * - Attacking: Melee attack when dash ends near player
	 * - GettingHit: Stagger animation when damaged
	 * - Dying: Death animation, then marked as dead
	 * 
	 * ## Behavior
	 * - Starts idle, activates when player enters VISION_RANGE
	 * - When in DASH_TRIGGER_RANGE, prepares dash (locks target position)
	 * - Dashes to locked position at high speed
	 * - If player still nearby after dash, attacks
	 * - Dash has cooldown between uses
	 */
	class Devil : public EnemyInterface {
	public:
		Devil(float x, float y, Core::Map* map);

		void update(float dt) override;
		void takeDamage(int dmg) override;
		
		void setTarget(const std::shared_ptr<Entity>& target) { _target = target; }

	private:
		enum class State { Idle, Chasing, PreparingDash, Dashing, Recovering, Attacking, Dying };
		State _state = State::Idle;


		std::weak_ptr<Entity> _target;

		//Animation speed set
		static constexpr float DEVIL_DASH_ANIMATION_SPEED = 3.0f;
		static constexpr float DEVIL_WALK_ANIMATION_SPEED = 1.0f;
		static constexpr float DEVIL_DEAD_ANIMATION_SPEED = 2.0f;
		static constexpr float DEVIL_ATTACK_ANIMATION_SPEED = 1.0f;
		
		// Combat stats
		static constexpr float VISION_RANGE = 8.0f;
		static constexpr float ATTACK_RANGE = 1.2f;
		static constexpr float SPEED = 0.5f;
		static constexpr int ATTACK_DAMAGE = 50;
		static constexpr float ATTACK_COOLDOWN = 1.5f;
		
		// Dash stats
		static constexpr float DASH_TRIGGER_RANGE = 5.0f;   // Distance at which dash can trigger
		static constexpr float DASH_SPEED = 6.0f;           // Dash speed
		static constexpr float DASH_COOLDOWN = 4.0f;        // Seconds between dashes
		static constexpr float DASH_PREPARE_TIME = 0.5f;    // Telegraph time before dash
		static constexpr float DASH_ARRIVE_THRESHOLD = 0.3f;// Distance to consider arrived
		static constexpr int DASH_DAMAGE = 35;              // Damage dealt when dash hits
		static constexpr float DASH_HIT_RANGE = 1.5f;       // Range to check collision during dash
		static constexpr float DASH_STUN_DURATION = 2.0f;   // Stun duration after dash ends
		
		float _attack_cooldown_timer = 0.0f;
		float _dash_cooldown_timer = 0.0f;
		float _dash_prepare_timer = 0.0f;
		Vector2 _dash_target_pos = {0, 0};  // Locked position for dash
		bool _dash_hit_target = false;      // Did we already hit during this dash?
		float _stun_timer = 0.0f;           // Recovery/stun timer after dash
	
		// Pathfinding
		static constexpr float PATH_RECALC_INTERVAL = 0.5f;
		float _path_recalc_timer = 0.0f;

		// State handlers
		void handleIdleState(float dt);
		void handleChasingState(float dt);
		void handlePreparingDashState(float dt);
		void handleDashingState(float dt);
		void handleRecoveringState(float dt);
		void handleAttackingState(float dt);

		void handleDyingState(float dt);
		
		// Utility
		float getDistanceToTarget() const;
	};

} // namespace Nawia::Entity
