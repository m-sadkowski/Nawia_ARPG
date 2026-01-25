#include "Devil.h"
#include "Collider.h"

#include <MathUtils.h>
#include <raymath.h>

namespace Nawia::Entity {

	Devil::Devil(const float x, const float y, Core::Map* map)
		: EnemyInterface("Devil", x, y, nullptr, 120, map)
	{
		setScale(0.04f);
		setFaction(Faction::Enemy);

		loadModel("../assets/models/devil_idle.glb");
		addAnimation("idle", "../assets/models/devil_idle.glb");	
		addAnimation("walk", "../assets/models/devil_walk.glb");
		addAnimation("run", "../assets/models/devil_run.glb");
		addAnimation("attack", "../assets/models/devil_attack.glb");

		// TODO

		addAnimation("death", "../assets/models/devil_idle.glb");

		// TODO 
		addAnimation("get_hit", "../assets/models/devil_idle.glb");

		setCollider(std::make_unique<RectangleCollider>(this, 0.5f, 1.f, -2.1f, -1.f));
	}

	void Devil::takeDamage(const int dmg)
	{
		if (_state == State::Dying) return;
		// Can't interrupt dash with damage
		if (_state == State::Dashing) return;

		if (_hp - dmg <= 0)
		{
			_hp = 1;
			_state = State::Dying;
			playAnimation("death", false, true, 0, true);
			setFaction(Faction::None);
		}
		else
		{
			Entity::takeDamage(dmg);
			
			if (_state != State::GettingHit)
				_state_before_hit = _state;
			
			_state = State::GettingHit;
			playAnimation("get_hit", false, true, 10, true);
			setVelocity(0, 0);
		}
	}

	void Devil::update(const float dt)
	{
		if (_attack_cooldown_timer > 0.0f)
			_attack_cooldown_timer -= dt;
		if (_dash_cooldown_timer > 0.0f)
			_dash_cooldown_timer -= dt;

		switch (_state)
		{
		case State::Idle:
			handleIdleState(dt);
			break;
		case State::Chasing:
			handleChasingState(dt);
			break;
		case State::PreparingDash:
			handlePreparingDashState(dt);
			break;
		case State::Dashing:
			handleDashingState(dt);
			break;
		case State::Recovering:
			handleRecoveringState(dt);
			break;
		case State::Attacking:
			handleAttackingState(dt);
			break;
		case State::GettingHit:
			handleGettingHitState(dt);
			break;
		case State::Dying:
			handleDyingState(dt);
			break;
		}
	}

	void Devil::handleIdleState(const float dt)
	{
		Entity::update(dt);

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

	void Devil::handleChasingState(const float dt)
	{
		EnemyInterface::update(dt);

		auto target = _target.lock();
		if (!target || target->isDead())
		{
			_state = State::Idle;
			playAnimation("idle");
			setVelocity(0, 0);
			return;
		}

		const float dist = getDistanceToTarget();

		// Return to idle if player escaped
		if (dist > VISION_RANGE * 1.5f)
		{
			_state = State::Idle;
			playAnimation("idle");
			setVelocity(0, 0);
			return;
		}

		// Try to dash if in range and off cooldown
		if (dist <= DASH_TRIGGER_RANGE && dist > ATTACK_RANGE && _dash_cooldown_timer <= 0.0f)
		{
			// Lock target position NOW - player can dodge after this!
			const Vector2 target_pos = target->getCollider() 
				? target->getCollider()->getPosition() 
				: target->getCenter();
			_dash_target_pos = target_pos;
			
			_state = State::PreparingDash;
			_dash_prepare_timer = DASH_PREPARE_TIME;
			setVelocity(0, 0);
			// Brief pause - could add a "wind-up" animation here
			playAnimation("idle");
			return;
		}

		// Attack if in range
		if (dist <= ATTACK_RANGE && _attack_cooldown_timer <= 0.0f)
		{
			_state = State::Attacking;
			playAnimation("attack", false, true);
			setVelocity(0, 0);
			return;
		}

		// Move towards player
		rotateTowards(target->getX(), target->getY());
		
		const Vector2 my_pos = getCollider() ? getCollider()->getPosition() : _pos;
		const Vector2 target_pos = target->getCollider() ? target->getCollider()->getPosition() : target->getCenter();
		
		const Vector2 dir = Vector2Normalize(Vector2Subtract(target_pos, my_pos));
		
		_pos.x += dir.x * SPEED * dt;
		_pos.y += dir.y * SPEED * dt;
	}

	void Devil::handlePreparingDashState(const float dt)
	{
		Entity::update(dt);
		
		// Telegraph phase - devil pauses before charging
		_dash_prepare_timer -= dt;
		
		// Face the locked target position
		rotateTowards(_dash_target_pos.x, _dash_target_pos.y);
		
		if (_dash_prepare_timer <= 0.0f)
		{
			// Start the actual dash!
			_state = State::Dashing;
			_dash_hit_target = false;  // Reset hit flag for new dash
			playAnimation("run", false, false);  // Run animation for dash
		}
	}

	void Devil::handleDashingState(const float dt)
	{
		Entity::update(dt);

		const Vector2 my_pos = getCollider() ? getCollider()->getPosition() : _pos;
		const float dist_to_dash_target = Vector2Distance(my_pos, _dash_target_pos);
		
		// Check for collision with target during dash (deal damage once)
		if (!_dash_hit_target)
		{
			if (const auto target = _target.lock())
			{
				const float dist_to_player = getDistanceToTarget();
				if (dist_to_player <= DASH_HIT_RANGE)
				{
					// Hit! Deal dash damage
					target->takeDamage(DASH_DAMAGE);
					_dash_hit_target = true;
				}
			}
		}
		
		// Arrived at dash destination?
		if (dist_to_dash_target <= DASH_ARRIVE_THRESHOLD)
		{
			_dash_cooldown_timer = DASH_COOLDOWN;
			
			// Dash finished - enter stun/recovery state
			_state = State::Recovering;
			_stun_timer = DASH_STUN_DURATION;
			playAnimation("idle");  // Stunned animation (or idle)
			return;
		}
		
		// Move towards locked position at high speed
		const Vector2 dir = Vector2Normalize(Vector2Subtract(_dash_target_pos, my_pos));
		
		_pos.x += dir.x * DASH_SPEED * dt;
		_pos.y += dir.y * DASH_SPEED * dt;
	}

	void Devil::handleRecoveringState(const float dt)
	{
		Entity::update(dt);
		
		// Stunned after dash - can't move or act
		_stun_timer -= dt;
		
		if (_stun_timer <= 0.0f)
		{
			// Recovery finished - back to chasing
			_state = State::Chasing;
			playAnimation("walk");
		}
	}

	void Devil::handleAttackingState(const float dt)
	{
		Entity::update(dt);

		if (!isAnimationLocked())
		{
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

	void Devil::handleGettingHitState(const float dt)
	{
		Entity::update(dt);
		
		if (!isAnimationLocked())
		{
			_state = _state_before_hit;
			
			switch (_state)
			{
			case State::Idle:
				playAnimation("idle");
				break;
			case State::Chasing:
			case State::PreparingDash:
				_state = State::Chasing;
				playAnimation("walk");
				break;
			case State::Attacking:
				_state = State::Chasing;
				playAnimation("walk");
				break;
			default:
				playAnimation("idle");
				break;
			}
		}
	}

	void Devil::handleDyingState(const float dt)
	{
		Entity::update(dt);
		
		if (!isAnimationLocked())
		{
			_hp = 0;
		}
	}

	float Devil::getDistanceToTarget() const
	{
		const auto target = _target.lock();
		if (!target) return std::numeric_limits<float>::max();

		const Vector2 my_pos = getCollider() ? getCollider()->getPosition() : _pos;
		const Vector2 target_pos = target->getCollider() ? target->getCollider()->getPosition() : target->getCenter();
		
		return Vector2Distance(my_pos, target_pos);
	}

} // namespace Nawia::Entity
