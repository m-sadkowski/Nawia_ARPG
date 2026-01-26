#include "Bandit.h"
#include "Ability.h"
#include "KnifeThrowAbility.h"
#include "Collider.h"

#include <MathUtils.h>
#include <raymath.h>

namespace Nawia::Entity {

	Bandit::Bandit(const float x, const float y, Core::Map* map)
		: EnemyInterface("Bandit", x, y, nullptr, 80, map)
	{
		setScale(0.03f);
		setFaction(Faction::Enemy);

		// TODO: Update paths to actual bandit model files
		loadModel("../assets/models/bandit_idle.glb");
		addAnimation("idle", "../assets/models/bandit_idle.glb");
		addAnimation("walk", "../assets/models/bandit_walk.glb");
		addAnimation("throw", "../assets/models/bandit_throw.glb");
		addAnimation("death", "../assets/models/bandit_death.glb");
		addAnimation("get_hit", "../assets/models/bandit_hit.glb");

		setCollider(std::make_unique<RectangleCollider>(this, 0.3f, 0.8f, -2.1f, -1.f));

		
	}

	void Bandit::takeDamage(const int dmg)
	{
		if (_state == State::Dying) return;

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

	void Bandit::update(const float dt)
	{
		if (_knife_cooldown_timer > 0.0f)
			_knife_cooldown_timer -= dt;

		switch (_state)
		{
		case State::Idle:
			handleIdleState(dt);
			break;
		case State::Chasing:
			handleChasingState(dt);
			break;
		case State::Casting:
			handleCastingState(dt);
			break;
		case State::GettingHit:
			handleGettingHitState(dt);
			break;
		case State::Dying:
			handleDyingState(dt);
			break;
		}
	}

	void Bandit::handleIdleState(const float dt)
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

	void Bandit::handleChasingState(const float dt)
	{
		EnemyInterface::update(dt);
		updateAbilities(dt);

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

		// Try to throw knife if in range and off cooldown
		if (dist <= ATTACK_RANGE && _knife_cooldown_timer <= 0.0f)
		{
			if (const auto knife = getAbility(0))
			{
				if (knife->isReady())
				{
					_state = State::Casting;
					playAnimation("throw", false, true);
					rotateTowards(target->getX(), target->getY());
					return;
				}
			}
		}

		// Move toward or away from player to maintain optimal distance
		const Vector2 my_pos = getCollider() ? getCollider()->getPosition() : _pos;
		const Vector2 target_pos = target->getCollider() ? target->getCollider()->getPosition() : target->getCenter();
		
		Vector2 dir = Vector2Normalize(Vector2Subtract(target_pos, my_pos));
		
		// If too close, back away
		if (dist < MIN_DISTANCE)
		{
			dir = Vector2Negate(dir);  // Reverse direction
		}
		// If in good range, stop moving
		else if (dist >= MIN_DISTANCE && dist <= ATTACK_RANGE)
		{
			rotateTowards(target->getX(), target->getY());
			playAnimation("idle");
			return;
		}
		
		rotateTowards(target->getX(), target->getY());
		
		_pos.x += dir.x * SPEED * dt;
		_pos.y += dir.y * SPEED * dt;
	}

	void Bandit::handleCastingState(const float dt)
	{
		Entity::update(dt);
		updateAbilities(dt);

		if (auto target = _target.lock())
		{
			rotateTowards(target->getX(), target->getY());
		}

		// Trigger the throw at exactly frame 60
		if (_anim_frame_counter == 60)
		{
			if (const auto knife = getAbility(0))
			{
				if (auto target = _target.lock())
				{
					const float tx = target->getCenter().x;
					const float ty = target->getCenter().y;

					// Only cast if we haven't already just casted (though frame counter check handles this mostly)
					// We rely on the frame counter being exactly 60 for one frame.
					if (auto effect = knife->cast(tx, ty))
					{
						addPendingSpawn(std::move(effect));
						_knife_cooldown_timer = KNIFE_COOLDOWN;
					}
				}
			}
		}
		
		if (!isAnimationLocked())
		{
			_state = State::Chasing;
			playAnimation("walk");
		}
	}

	void Bandit::handleGettingHitState(const float dt)
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
			case State::Casting:
				
			default:
				playAnimation("idle");
				break;
			}
		}
	}

	void Bandit::handleDyingState(const float dt)
	{
		Entity::update(dt);
		
		if (!isAnimationLocked())
		{
			_hp = 0;
		}
	}

	float Bandit::getDistanceToTarget() const
	{
		const auto target = _target.lock();
		if (!target) return std::numeric_limits<float>::max();

		const Vector2 my_pos = getCollider() ? getCollider()->getPosition() : _pos;
		const Vector2 target_pos = target->getCollider() ? target->getCollider()->getPosition() : target->getCenter();
		
		return Vector2Distance(my_pos, target_pos);
	}

} // namespace Nawia::Entity
