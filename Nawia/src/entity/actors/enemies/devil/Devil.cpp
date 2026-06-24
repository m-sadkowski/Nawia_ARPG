#include "Devil.h"

#include <Collider.h>
#include <Map.h>
#include <MathUtils.h>
#include <Player.h>
#include <SoundIds.h>

#include <raymath.h>

namespace Nawia::Entity {

	Devil::Devil() {
		setScale(0.025f);
		setFaction(Faction::Enemy);

		loadModel("assets/models/actors/devil/devil_idle.glb");
		addAnimation("idle", "assets/models/actors/devil/devil_idle.glb");
		addAnimation("walk", "assets/models/actors/devil/devil_walk.glb");
		addAnimation("run", "assets/models/actors/devil/devil_run.glb");
		addAnimation("attack", "assets/models/actors/devil/devil_attack.glb");
		addAnimation("death", "assets/models/actors/devil/devil_dead.glb");
		setMovementSpeed(SPEED);
	}

	Devil::Devil(const float x, const float y, Core::Map* map)
		: EnemyInterface("Devil", x, y, nullptr, 120, map)
	{
		setScale(0.025f);
		setFaction(Faction::Enemy);

		loadModel("assets/models/actors/devil/devil_idle.glb");
		addAnimation("idle", "assets/models/actors/devil/devil_idle.glb");
		addAnimation("walk", "assets/models/actors/devil/devil_walk.glb");
		addAnimation("run", "assets/models/actors/devil/devil_run.glb");
		addAnimation("attack", "assets/models/actors/devil/devil_attack.glb");
		addAnimation("death", "assets/models/actors/devil/devil_dead.glb");

		setCollider(std::make_unique<RectangleCollider>(this, 1.f, 1.2f, 0.0f, 0.0f));
		setMovementSpeed(SPEED);
	}

	void Devil::update(const float dt)
	{
		updateMovementSound(Audio::SoundPath::DevilStep, _is_moving && !isDying() && !isDormant(), 0.45f);


		if (isDying())
		{
			Entity::update(dt);
			return;
		}

		if (isDormant()) return;

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
				playSoundEffect(Audio::SoundId::DevilAggro, 0.9f);
				setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
				playAnimation("walk");
			}
		}
	}

	void Devil::handleChasingState(const float dt)
	{
		Entity::update(dt);  // Bazowa aktualizacja animacji.

		auto target = _target.lock();
		if (!target || target->isDead())
		{
			_state = State::Idle;
			setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
			playAnimation("idle");
			setVelocity(0, 0);
			_is_moving = false;
			return;
		}

		const float dist = getDistanceToTarget();


		if (dist > VISION_RANGE * 1.5f)
		{
			_state = State::Idle;
			setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
			playAnimation("idle");
			setVelocity(0, 0);
			_is_moving = false;
			return;
		}


		// Sprawdzenie, czy cel jest w zasięgu ataku.
		if (dist <= ATTACK_RANGE && _attack_cooldown_timer <= 0.0f)
		{
			_state = State::Attacking;
			setAnimationSpeed(DEVIL_ATTACK_ANIMATION_SPEED);
			playAnimation("attack", false, true);
			setVelocity(0, 0);
			_is_moving = false;
			return;
		}

		const Vector2 target_pos = target->getCenter();

		// Logika doskoku: cel w zasięgu, czas odnowienia gotowy i trasa nie jest zablokowana.
		if (dist <= DASH_TRIGGER_RANGE && dist > ATTACK_RANGE && _dash_cooldown_timer <= 0.0f)
		{
			_dash_target_pos = target_pos;
			_state = State::PreparingDash;
			_dash_prepare_timer = DASH_PREPARE_TIME;
			setVelocity(0, 0);
			_is_moving = false;
			setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
			playAnimation("idle");
			return;
		}

		// Zwykły pościg z okresowym odświeżaniem celu ruchu.
		_path_recalc_timer -= dt;

		if (_path_recalc_timer <= 0.0f || !_is_moving)
		{
			moveTo(target->getX(), target->getY());
			_path_recalc_timer = DEFAULT_PATH_RECALC_INTERVAL;
		}

		updateMovement(dt);

		if (_is_moving)
		{
			setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
			playAnimation("walk");
		}
	}

	void Devil::handlePreparingDashState(const float dt)
	{
		Entity::update(dt);


		_dash_prepare_timer -= dt;


		rotateTowards(_dash_target_pos.x, _dash_target_pos.y);

		if (_dash_prepare_timer <= 0.0f)
		{

			_state = State::Dashing;
			_dash_hit_target = false;
			playSoundEffect(Audio::SoundId::DevilDash, 0.9f);
			setAnimationSpeed(DEVIL_DASH_ANIMATION_SPEED);
			playAnimation("run", true, false);
		}
	}

	void Devil::handleDashingState(const float dt)
	{
		Entity::update(dt);
		const Vector2 my_pos = getCenter();
		const float dist_to_dash_target = Vector2Distance(my_pos, _dash_target_pos);
		if (!_dash_hit_target)
		{
			if (const auto target = _target.lock())
			{
				const float dist_to_player = getDistanceToTarget();
				if (dist_to_player <= DASH_HIT_RANGE)
				{
					// Gracza próbujemy powalić, pozostałym celom zadajemy zwykłe obrażenia.
					target->rememberDamageSource(this);
					if (target->getType() == EntityType::Player)
						dynamic_cast<Player*>(target.get())->knockDown(static_cast<int>(DASH_DAMAGE * _damage_multiplier));
					else
						target->takeDamage(static_cast<int>(DASH_DAMAGE * _damage_multiplier));
					playSoundEffect(Audio::SoundId::DevilDashHit, 0.95f);
					_dash_hit_target = true;

				}
			}
		}

		if (dist_to_dash_target <= DASH_ARRIVE_THRESHOLD)
		{
			_dash_cooldown_timer = DASH_COOLDOWN;


			_state = State::Recovering;
			_stun_timer = DASH_STUN_DURATION;
			setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
			playAnimation("idle");
			return;
		}

		const Vector2 dir = Vector2Normalize(Vector2Subtract(_dash_target_pos, my_pos));
		const float move_amount = DASH_SPEED * _speed_multiplier * dt;

		const float next_x = _pos.x + dir.x * move_amount;
		const float next_y = _pos.y + dir.y * move_amount;

		const Vector2 center = getCenter();
		const float offset_x = center.x - getX();
		const float offset_y = center.y - getY();
		const float next_center_x = next_x + offset_x;
		const float next_center_y = next_y + offset_y;

		if (_map && !_map->isWalkable(next_center_x, next_center_y))
		{
			// Doskok uderzył w ścianę lub niewalkowalny fragment mapy.
			_dash_cooldown_timer = DASH_COOLDOWN;
			_state = State::Recovering;
			_stun_timer = DASH_STUN_DURATION;
			setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
			playAnimation("idle");
			return;
		}

		_pos.x = next_x;
		_pos.y = next_y;
	}

	void Devil::handleRecoveringState(const float dt)
	{
		Entity::update(dt);
		_stun_timer -= dt;

		if (_stun_timer <= 0.0f)
		{
			_state = State::Chasing;
			setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
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
					target->rememberDamageSource(this);
					target->takeDamage(static_cast<int>(ATTACK_DAMAGE * _damage_multiplier));
					playSoundEffect(Audio::SoundId::DevilPunch, 0.85f);

				}
			}

			_attack_cooldown_timer = ATTACK_COOLDOWN;
			_state = State::Chasing;
			setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
			playAnimation("walk");
		}
	}

	void Devil::onDeathStarted()
	{
		playSoundEffect(Audio::SoundId::DevilDeath, 0.95f);
	}
} // namespace Nawia::Entity




