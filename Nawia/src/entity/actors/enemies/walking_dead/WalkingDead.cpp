#include "WalkingDead.h"
#include "Collider.h"

#include <MathUtils.h>
#include <raymath.h>

namespace Nawia::Entity {

	WalkingDead::WalkingDead() {
		setScale(0.015f);
		setFaction(Faction::Enemy);

		loadModel("../assets/models/walking_dead_idle.glb");
		addAnimation("idle", "../assets/models/walking_dead_idle.glb");
		addAnimation("walk", "../assets/models/walking_dead_walk.glb");
		addAnimation("run", "../assets/models/walking_dead_run.glb");
		addAnimation("attack", "../assets/models/walking_dead_attack.glb");
		addAnimation("death", "../assets/models/walking_dead_death.glb");
		addAnimation("scream", "../assets/models/walking_dead_scream.glb");
		addAnimation("get_hit", "../assets/models/walking_dead_hit.glb");
	}

	void WalkingDead::takeDamage(const int dmg)
	{
		Entity::takeDamage(dmg);
		if (isDying()) return;

		// When hit, stagger the zombie, interrupting its current action
		if (_state != State::GettingHit)
			_state_before_hit = _state;
		
		_state = State::GettingHit;
		playAnimation("get_hit", false, true, 10, true);
		
		// Stop movement while staggering
		setVelocity(0, 0);
	}

	void WalkingDead::update(const float dt)
	{
		if (isDying())
		{
			Entity::update(dt);
			return;
		}

		if (isDormant()) return;

		// Update attack cooldown
		if (_attack_cooldown_timer > 0.0f)
			_attack_cooldown_timer -= dt;

		switch (_state)
		{
		case State::Idle:
			handleIdleState(dt);
			break;
		case State::Chasing:
			handleChasingState(dt);
			break;
		case State::Attacking:
			handleAttackingState(dt);
			break;
		case State::Screaming:
			handleScreamingState(dt);
			break;
		case State::GettingHit:
			handleGettingHitState(dt);
			break;
		}
	}

	void WalkingDead::handleIdleState(const float dt)
	{
		Entity::update(dt);

		// Try to find player target
		if (auto target = _target.lock())
		{
			const float dist = getDistanceToTarget();
			if (dist <= VISION_RANGE)
			{
				_state = State::Chasing;
				playAnimation("walk");
			}
		}
	}

	void WalkingDead::handleChasingState(const float dt)
	{
		Entity::update(dt);  // Base update for animations

		auto target = _target.lock();
		if (!target || target->isDead())
		{
			_state = State::Screaming;
			playAnimation("scream", false, true);
			setVelocity(0, 0);
			_is_moving = false;
			return;
		}

		const float dist = getDistanceToTarget();

		// Check if player escaped vision range
		if (dist > VISION_RANGE * 1.5f)
		{
			_state = State::Screaming;
			playAnimation("scream", false, true);
			setVelocity(0, 0);
			_is_moving = false;
			return;
		}

		// Check if in attack range
		if (dist <= ATTACK_RANGE && _attack_cooldown_timer <= 0.0f)
		{
			_state = State::Attacking;
			playAnimation("attack", false, true);
			setVelocity(0, 0);
			_is_moving = false;
			return;
		}

		// Determine speed based on distance
		const bool should_run = dist <= CLOSE_RANGE;
		if (should_run != _is_running)
		{
			_is_running = should_run;
			playAnimation(_is_running ? "run" : "walk");
		}
		
		const float current_speed = _is_running ? RUN_SPEED : SPEED;
		setMovementSpeed(current_speed);
		
		// Get target position
		const Vector2 target_pos = target->getCenter();
		
		// If close enough, use direct movement with validation
		if (dist <= DIRECT_MOVE_DISTANCE)
		{
			_is_moving = false;  // Stop pathfinding movement
			const Vector2 my_pos = getCenter();
			const Vector2 dir = Vector2Normalize(Vector2Subtract(target_pos, my_pos));
			
			rotateTowards(target->getX(), target->getY());
			
			_pos.x += dir.x * current_speed * dt;
			_pos.y += dir.y * current_speed * dt;
		}
		else
		{
			// Use A* pathfinding when farther away
			_path_recalc_timer -= dt;
			
			if (_path_recalc_timer <= 0.0f || !_is_moving)
			{
				moveTo(target_pos.x, target_pos.y);
				_path_recalc_timer = DEFAULT_PATH_RECALC_INTERVAL;
			}
			
			updateMovement(dt);
		}
	}

	void WalkingDead::handleAttackingState(const float dt)
	{
		Entity::update(dt);

		if (!isAnimationLocked())
		{
			// Attack animation finished - deal damage
			if (const auto target = _target.lock())
			{
				if (getDistanceToTarget() <= ATTACK_RANGE * 1.5f)
				{
					target->takeDamage(ATTACK_DAMAGE);
				}
			}
			
			_attack_cooldown_timer = ATTACK_COOLDOWN;
			_state = State::Chasing;
			playAnimation("walk");
		}
	}

	void WalkingDead::handleScreamingState(const float dt)
	{
		Entity::update(dt);
		
		if (!isAnimationLocked())
		{
			// Scream animation finished - return to idle
			_state = State::Idle;
			playAnimation("idle");
			_is_running = false;
		}
	}

	void WalkingDead::handleGettingHitState(const float dt)
	{
		Entity::update(dt);
		
		if (!isAnimationLocked())
		{
			// Get_hit animation finished - return to previous state
			_state = _state_before_hit;
			
			// Resume appropriate animation based on state
			switch (_state)
			{
			case State::Idle:
				playAnimation("idle");
				break;
			case State::Chasing:
				playAnimation(_is_running ? "run" : "walk");
				break;
			case State::Attacking:
				// If we were attacking, go back to chasing
				_state = State::Chasing;
				playAnimation(_is_running ? "run" : "walk");
				break;
			case State::Screaming:
				playAnimation("scream", false, true);
				break;
			default:
				playAnimation("idle");
				break;
			}
		}
	}

} // namespace Nawia::Entity
