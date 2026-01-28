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
		setScale(0.015f);
		setFaction(Faction::Enemy);
		
		loadModel("../assets/models/bandit_idle.glb");
		addAnimation("idle", "../assets/models/bandit_idle.glb");
		addAnimation("walk", "../assets/models/bandit_walk_backwards3.glb");
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
				playAnimation("idle");
			}
		}
	}

	void Bandit::handleChasingState(const float dt) {
		Entity::update(dt);  // Base update for animations
		updateAbilities(dt);

		auto target = _target.lock();
		if (!target || target->isDead()) {
			_state = State::Idle;
			playAnimation("idle");
			setVelocity(0, 0);
			_is_moving = false;
			return;
		}

		const float dist = getDistanceToTarget();

		if (dist > VISION_RANGE * 1.5f) {
			_state = State::Idle;
			playAnimation("idle");
			setVelocity(0, 0);
			_is_moving = false;
			return;
		}

		// Ready to throw knife?
		if (dist <= ATTACK_RANGE && _knife_cooldown_timer <= 0.0f) {
			if (const auto knife = getAbility(0)) {
				if (knife->isReady()) {
					_state = State::Casting;
					_knife_thrown_this_cast = false;  // reset flag for new cast
					setAnimationSpeed(1.5f);
					playAnimation("throw", false, true);
					rotateTowards(target->getX(), target->getY());
					setVelocity(0, 0);
					_is_moving = false;
					return;
				}
			}
		}

		const Vector2 my_pos = getCollider() ? getCollider()->getPosition() : _pos;
		const Vector2 target_pos = target->getCollider() ? target->getCollider()->getPosition() : target->getCenter();
		
		setMovementSpeed(SPEED);
		
		// Too close - retreat using pathfinding
		if (dist < MIN_DISTANCE) 
		{
			_path_recalc_timer -= dt;
			
			if (_path_recalc_timer <= 0.0f || !_is_moving || !_is_retreating) 
			{
				// Find a retreat point - 3 tiles away from player, opposite direction
				const Vector2 away_dir = Vector2Normalize(Vector2Subtract(my_pos, target_pos));
				const float retreat_dist = 3.0f;
				
				// Try to find a walkable retreat point
				Vector2 retreat_point = { my_pos.x + away_dir.x * retreat_dist, my_pos.y + away_dir.y * retreat_dist };
				
				// If not walkable, try smaller distances or angles
				if (_map && !_map->isWalkable(retreat_point.x, retreat_point.y))
				{
					// Try 45 degree offsets
					for (const float angle_offset : { 0.785f, -0.785f, 1.57f, -1.57f }) 
					{
						const float angle = std::atan2(away_dir.y, away_dir.x) + angle_offset;
						retreat_point = { my_pos.x + std::cos(angle) * retreat_dist, my_pos.y + std::sin(angle) * retreat_dist };
						if (_map->isWalkable(retreat_point.x, retreat_point.y)) break;
					}
				}
				
				moveTo(retreat_point.x, retreat_point.y);
				_path_recalc_timer = PATH_RECALC_INTERVAL;
				_is_retreating = true;
			}
			
			playAnimation("walk");
			updateMovement(dt);
			rotateTowards(target->getX(), target->getY());  // Face target while retreating
		}
		// Too far - approach using pathfinding
		else if (dist > ATTACK_RANGE) 
		{
			_path_recalc_timer -= dt;
			
			if (_path_recalc_timer <= 0.0f || !_is_moving || _is_retreating) 
			{
				moveTo(target_pos.x, target_pos.y);
				_path_recalc_timer = PATH_RECALC_INTERVAL;
				_is_retreating = false;
			}
			
			playAnimation("walk");
			updateMovement(dt);
		}
		// In sweet spot - stand and face target
		else 
		{
			playAnimation("idle");
			_is_moving = false;
			_is_retreating = false;
		}
		
		rotateTowards(target->getX(), target->getY());
	}

	void Bandit::handleCastingState(const float dt)
	{
		Entity::update(dt);
		updateAbilities(dt);

		// keep rotating towards player during cast animation
		if (auto target = _target.lock())
		{
			rotateTowards(target->getX(), target->getY());
		}

		// throw knife once during animation (at frame 60)
		if (_anim_frame_counter > 60 && !_knife_thrown_this_cast)
		{
			if (const auto knife = getAbility(0))
			{
				if (const auto target = _target.lock())
				{
					const float tx = target->getCenter().x;
					const float ty = target->getCenter().y;

					if (auto effect = knife->cast(tx, ty))
					{
						addPendingSpawn(std::move(effect));
						_knife_cooldown_timer = KNIFE_COOLDOWN;
						_knife_thrown_this_cast = true;  // mark as thrown for this cast
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
