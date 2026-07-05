#include "Devil.h"

#include <BossTelegraphHazard.h>
#include <Collider.h>
#include <Map.h>
#include <MathUtils.h>
#include <Player.h>
#include <SoundIds.h>

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

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
		updateMovementSound(Audio::SoundPath::DevilStep, isMoving() && !isDying() && !isDormant(), 0.45f);


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
		case State::JumpingShockwave:
			handleJumpingShockwaveState(dt);
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
			stopMovement();
			return;
		}

		const float dist = getDistanceToTarget();


		if (dist > VISION_RANGE * 1.5f)
		{
			_state = State::Idle;
			setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
			playAnimation("idle");
			setVelocity(0, 0);
			stopMovement();
			return;
		}


		// Sprawdzenie, czy cel jest w zasięgu ataku.
		if (dist <= ATTACK_RANGE && _attack_cooldown_timer <= 0.0f)
		{
			startAttack();
			return;
		}

		const Vector2 target_pos = target->getCenter();

		// Logika doskoku: cel w zasięgu, czas odnowienia gotowy i trasa nie jest zablokowana.
		if (dist <= DASH_TRIGGER_RANGE && dist > ATTACK_RANGE && _dash_cooldown_timer <= 0.0f)
		{
			if (isBossVariant() &&
				dist <= SHOCKWAVE_TRIGGER_RANGE &&
				GetRandomValue(0, 99) < SHOCKWAVE_CHANCE_PERCENT) {
				startShockwaveJump();
			} else {
				startDash(target_pos);
			}
			return;
		}

		// Zwykły pościg z okresowym odświeżaniem celu ruchu.
		tickPathRecalcTimer(dt);

		if (isPathRecalcDue() || !isMoving())
		{
			moveTo(target->getX(), target->getY());
			resetPathRecalcTimer(DEFAULT_PATH_RECALC_INTERVAL);
		}

		updateMovement(dt);

		if (isMoving())
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
			clearCastTelemetry();
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
					target->rememberDamageSource(this, "Devil Dash");
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
			spawnScorchedGroundHazard();
			_dash_cooldown_timer = DASH_COOLDOWN;


			_state = State::Recovering;
			_stun_timer = dashRecoveryDuration();
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
			spawnScorchedGroundHazard();
			_dash_cooldown_timer = DASH_COOLDOWN;
			_state = State::Recovering;
			_stun_timer = dashRecoveryDuration();
			setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
			playAnimation("idle");
			return;
		}

		_pos.x = next_x;
		_pos.y = next_y;
	}

	void Devil::handleJumpingShockwaveState(const float dt)
	{
		Entity::update(dt);
		setVelocity(0, 0);
		stopMovement();

		if (const auto target = _target.lock())
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);

		_shockwave_timer += dt;
		const float progress = std::clamp(_shockwave_timer / SHOCKWAVE_WINDUP_TIME, 0.0f, 1.0f);
		_visual_jump_height = std::sin(progress * 3.14159265f) * SHOCKWAVE_JUMP_HEIGHT;

		if (!_shockwave_impact_played && _shockwave_timer >= SHOCKWAVE_WINDUP_TIME) {
			_shockwave_impact_played = true;
			_visual_jump_height = 0.0f;
			clearCastTelemetry();
			playSoundEffect(Audio::SoundId::DevilPunch, 0.95f, true, 0.72f);
		}

		if (_shockwave_timer >= SHOCKWAVE_WINDUP_TIME) {
			_state = State::Recovering;
			_stun_timer = BOSS_SHOCKWAVE_RECOVERY_DURATION;
			setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
			playAnimation("idle");
		}
	}

	void Devil::handleRecoveringState(const float dt)
	{
		Entity::update(dt);
		_stun_timer -= dt;

		if (_stun_timer <= 0.0f)
			finishRecovery();
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
					target->rememberDamageSource(this, "Devil Punch");
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

	void Devil::startAttack()
	{
		_state = State::Attacking;
		setAnimationSpeed(DEVIL_ATTACK_ANIMATION_SPEED);
		playAnimation("attack", false, true);
		setVelocity(0, 0);
		stopMovement();
	}

	void Devil::startDash(const Vector2& target_pos)
	{
		_dash_target_pos = target_pos;
		_state = State::PreparingDash;
		_dash_prepare_timer = DASH_PREPARE_TIME;
		beginCastTelemetry("Devil Dash", DASH_PREPARE_TIME, false);
		spawnDashImpactHazard();
		setVelocity(0, 0);
		stopMovement();
		setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
		playAnimation("idle");
	}

	void Devil::startShockwaveJump()
	{
		_state = State::JumpingShockwave;
		_shockwave_timer = 0.0f;
		_shockwave_radius = static_cast<float>(GetRandomValue(
			static_cast<int>(SHOCKWAVE_MIN_RADIUS * 100.0f),
			static_cast<int>(SHOCKWAVE_MAX_RADIUS * 100.0f))) / 100.0f;
		_visual_jump_height = 0.0f;
		_shockwave_impact_played = false;
		_dash_cooldown_timer = DASH_COOLDOWN;
		_attack_cooldown_timer = std::max(_attack_cooldown_timer, 0.45f);
		beginCastTelemetry("Ground Slam", SHOCKWAVE_WINDUP_TIME, false);
		spawnShockwaveHazard();
		setVelocity(0, 0);
		stopMovement();
		setAnimationSpeed(DEVIL_SHOCKWAVE_IDLE_ANIMATION_SPEED);
		playAnimation("idle", true, false, 0, true);
	}

	void Devil::finishRecovery()
	{
		_visual_jump_height = 0.0f;
		if (const auto target = _target.lock()) {
			const float distance = getDistanceToTarget();
			if (!target->isDead() && distance <= ATTACK_RANGE * 1.5f && _attack_cooldown_timer <= 0.0f) {
				startAttack();
				return;
			}
		}

		_state = State::Chasing;
		setAnimationSpeed(DEVIL_WALK_ANIMATION_SPEED);
		playAnimation("walk");
	}

	float Devil::dashRecoveryDuration() const
	{
		return isBossVariant() ? BOSS_DASH_RECOVERY_DURATION : DASH_STUN_DURATION;
	}

	Vector3 Devil::getWorldPos3D() const
	{
		return {_pos.x, _altitude + _visual_jump_height, _pos.y};
	}

	bool Devil::isBossVariant() const {
		return getMaxHP() >= 180 || getName() == "Bies" || getName() == "Devil Lord";
	}

	void Devil::spawnDashImpactHazard() {
		if (!isBossVariant())
			return;

		BossTelegraphHazardConfig config;
		config.name = getName() + " Dash Impact";
		config.position = _dash_target_pos;
		config.altitude = getAltitude();
		config.radius = DASH_IMPACT_HAZARD_RADIUS;
		config.warning_seconds = DASH_PREPARE_TIME;
		config.active_seconds = 0.05f;
		config.damage_per_tick = 0;
		config.tick_interval = 0.45f;
		config.source_context = Entity::makeDamageSourceContext(this, config.name);
		config.warning_color = {255, 100, 20, 255};
		config.active_color = {230, 40, 20, 255};
		addPendingSpawn(std::make_shared<BossTelegraphHazard>(std::move(config)));
	}

	void Devil::spawnScorchedGroundHazard() {
		if (!isBossVariant())
			return;

		BossTelegraphHazardConfig config;
		config.name = getName() + " Scorched Ground";
		config.position = getCenter();
		config.altitude = getAltitude();
		config.radius = SCORCHED_GROUND_RADIUS;
		config.warning_seconds = 0.0f;
		config.active_seconds = SCORCHED_GROUND_DURATION;
		config.damage_per_tick = static_cast<int>(6.0f * _damage_multiplier);
		config.tick_interval = 0.75f;
		config.source_context = Entity::makeDamageSourceContext(this, config.name);
		config.warning_color = {255, 130, 30, 255};
		config.active_color = {190, 45, 18, 255};
		addPendingSpawn(std::make_shared<BossTelegraphHazard>(std::move(config)));
	}

	void Devil::spawnShockwaveHazard() {
		if (!isBossVariant())
			return;

		BossTelegraphHazardConfig config;
		config.name = getName() + " Ground Slam";
		config.position = getCenter();
		config.altitude = getAltitude();
		config.radius = _shockwave_radius;
		config.warning_seconds = SHOCKWAVE_WINDUP_TIME;
		config.active_seconds = (_shockwave_radius + SHOCKWAVE_WIDTH) / SHOCKWAVE_SPEED;
		config.damage_per_tick = static_cast<int>(SHOCKWAVE_DAMAGE * _damage_multiplier);
		config.tick_interval = 8.0f;
		config.knock_down_player_on_hit = true;
		config.expanding_wave = true;
		config.wave_speed = SHOCKWAVE_SPEED;
		config.wave_width = SHOCKWAVE_WIDTH;
		config.source_context = Entity::makeDamageSourceContext(this, config.name);
		config.warning_color = {255, 190, 55, 255};
		config.active_color = {245, 95, 25, 255};
		addPendingSpawn(std::make_shared<BossTelegraphHazard>(std::move(config)));
	}

	void Devil::onDeathStarted()
	{
		_visual_jump_height = 0.0f;
		playSoundEffect(Audio::SoundId::DevilDeath, 0.95f);
	}
} // namespace Nawia::Entity




