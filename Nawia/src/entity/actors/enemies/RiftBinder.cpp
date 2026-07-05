#include "RiftBinder.h"

#include <BossTelegraphHazard.h>
#include <Collider.h>
#include <FireRainHazard.h>
#include <Map.h>
#include <Player.h>
#include <Projectile.h>
#include <RiftTotem.h>
#include <SoundIds.h>

#include <raymath.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace Nawia::Entity {

	namespace {
		constexpr const char* DRAGON_MODEL = "assets/models/actors/dragon/dragon.glb";
		constexpr int DRAGON_ANIM_DEATH = 0;
		constexpr int DRAGON_ANIM_FAST_FLYING = 1;
		constexpr int DRAGON_ANIM_FLYING_IDLE = 2;
		constexpr int DRAGON_ANIM_HEADBUTT = 3;
		constexpr int DRAGON_ANIM_HIT_REACT = 4;
		constexpr int DRAGON_ANIM_NO = 5;
		constexpr int DRAGON_ANIM_PUNCH = 6;
		constexpr const char* STONE_PROJECTILE_MODEL = "assets/models/fireball.glb";
		constexpr float STONE_PROJECTILE_MODEL_SCALE = 0.3f;
		constexpr float MIN_DIRECTION_LENGTH_SQ = 0.0001f;
		constexpr Color STONE_PROJECTILE_TINT = {125, 125, 125, 255};
		constexpr std::array<int, 4> TOTEMS_BY_STAGE = {3, 4, 5, 7};
		constexpr std::array<float, 4> STAGE_THRESHOLDS = {1.0f, 0.75f, 0.50f, 0.25f};
		constexpr std::array<float, 4> FIRE_RAIN_STAGE_BASE_COOLDOWNS = {10.0f, 8.0f, 6.5f, 5.5f};
		constexpr std::array<float, 4> FIRE_RAIN_STAGE_MIN_COOLDOWNS = {8.5f, 6.5f, 5.5f, 4.0f};

		Vector2 safeNormalize(const Vector2 value, const Vector2 fallback)
		{
			if (Vector2LengthSqr(value) <= MIN_DIRECTION_LENGTH_SQ)
				return fallback;

			return Vector2Normalize(value);
		}

		float randomFloat(const float min_value, const float max_value)
		{
			const int min_scaled = static_cast<int>(std::round(min_value * 100.0f));
			const int max_scaled = static_cast<int>(std::round(max_value * 100.0f));
			return static_cast<float>(GetRandomValue(min_scaled, max_scaled)) / 100.0f;
		}

		int stageThresholdHp(const int max_hp, const int stage_index)
		{
			if (stage_index < 0 || stage_index >= static_cast<int>(STAGE_THRESHOLDS.size()))
				return 0;

			return static_cast<int>(std::ceil(static_cast<float>(max_hp) * STAGE_THRESHOLDS[stage_index]));
		}

		bool isFarEnoughFrom(
			const std::vector<Vector2>& positions,
			const Vector2 candidate,
			const float min_distance)
		{
			const float min_distance_sq = min_distance * min_distance;
			return std::ranges::all_of(positions, [candidate, min_distance_sq](const Vector2 existing) {
				return Vector2DistanceSqr(existing, candidate) >= min_distance_sq;
			});
		}
	}

	RiftBinder::RiftBinder()
	{
		setName("Siewca Chaosu");
		setMaxHp(420);
		setFaction(Faction::Enemy);
		setMovementSpeed(MOVE_SPEED);
		setCollider(std::make_unique<RectangleCollider>(this, 1.25f, 1.55f, 0.0f, 0.0f));
		configureModel();
	}

	RiftBinder::RiftBinder(const float x, const float y, Core::Map* map)
		: EnemyInterface("Siewca Chaosu", x, y, nullptr, 420, map)
	{
		setFaction(Faction::Enemy);
		setMovementSpeed(MOVE_SPEED);
		setCollider(std::make_unique<RectangleCollider>(this, 1.25f, 1.55f, 0.0f, 0.0f));
		configureModel();
	}

	void RiftBinder::configureModel()
	{
		loadModel(DRAGON_MODEL);
		if (hasModelLoaded())
			fitLoadedModelToHeight(DRAGON_TARGET_HEIGHT);
		addAnimation("death", DRAGON_MODEL, DRAGON_ANIM_DEATH);
		addAnimation("walk", DRAGON_MODEL, DRAGON_ANIM_FAST_FLYING);
		addAnimation("run", DRAGON_MODEL, DRAGON_ANIM_FAST_FLYING);
		addAnimation("idle", DRAGON_MODEL, DRAGON_ANIM_FLYING_IDLE);
		addAnimation("attack", DRAGON_MODEL, DRAGON_ANIM_HEADBUTT);
		addAnimation("fire_rain", DRAGON_MODEL, DRAGON_ANIM_HEADBUTT);
		addAnimation("get_hit", DRAGON_MODEL, DRAGON_ANIM_HIT_REACT);
		addAnimation("teleport", DRAGON_MODEL, DRAGON_ANIM_NO);
		addAnimation("stone_volley", DRAGON_MODEL, DRAGON_ANIM_PUNCH);
		playIdle();
	}

	void RiftBinder::update(const float dt)
	{
		updateMovementSound(Audio::SoundPath::DevilStep, isMoving() && !isDying() && !isDormant(), 0.38f, 0.75f);

		if (isDying()) {
			Entity::update(dt);
			return;
		}

		if (isDormant())
			return;

		updateTotemStage();

		if (_action_cooldown_timer > 0.0f)
			_action_cooldown_timer -= dt;
		if (_stone_cooldown_timer > 0.0f)
			_stone_cooldown_timer -= dt;
		if (_fire_rain_cooldown_timer > 0.0f)
			_fire_rain_cooldown_timer -= dt;
		if (_blink_cooldown_timer > 0.0f)
			_blink_cooldown_timer -= dt;
		if (_melee_cooldown_timer > 0.0f)
			_melee_cooldown_timer -= dt;

		if (canCastFireRain())
			_fire_rain_cooldown_timer = std::min(_fire_rain_cooldown_timer, currentFireRainCooldown());

		switch (_state) {
			case State::Idle:
				handleIdleState(dt);
				break;
			case State::Repositioning:
				handleRepositioningState(dt);
				break;
			case State::Casting:
				handleCastingState(dt);
				break;
			case State::Recovering:
				handleRecoveringState(dt);
				break;
			case State::MeleeAttacking:
				handleMeleeAttackingState(dt);
				break;
			case State::HitReacting:
				handleHitReactingState(dt);
				break;
		}
	}

	void RiftBinder::takeDamage(const int dmg)
	{
		if (dmg <= 0) {
			Entity::takeDamage(dmg);
			return;
		}

		if (_next_stage_to_start == 0) {
			startTotemStage(0);
			return;
		}

		if (_shield_active) {
			if (livingTotemCount() > 0) {
				playSoundEffect(Audio::SoundId::DevilPunch, 0.28f, true, 0.65f);
				return;
			}
			_shield_active = false;
			_active_stage = -1;
		}

		if (_next_stage_to_start > 0 && _next_stage_to_start < static_cast<int>(TOTEMS_BY_STAGE.size())) {
			const int threshold_hp = stageThresholdHp(getMaxHP(), _next_stage_to_start);
			if (getHP() > threshold_hp && getHP() - dmg <= threshold_hp) {
				const int damage_to_threshold = std::max(0, getHP() - threshold_hp);
				if (damage_to_threshold > 0)
					Entity::takeDamage(damage_to_threshold);

				if (!isDying() && !isDead())
					startTotemStage(_next_stage_to_start);
				return;
			}
		}

		Entity::takeDamage(dmg);
		startHitReact();
	}

	Vector3 RiftBinder::getWorldPos3D() const
	{
		return {getX(), getAltitude() + DRAGON_VISUAL_HEIGHT_OFFSET, getY()};
	}

	bool RiftBinder::isVisibleInCamera(const Camera3D& camera, const float screen_margin) const
	{
		(void)camera;
		(void)screen_margin;
		return !isDormant() && !isDead();
	}

	void RiftBinder::handleIdleState(const float dt)
	{
		Entity::update(dt);

		if (!hasValidTarget())
			return;

		if (getDistanceToTarget() <= VISION_RANGE) {
			_state = State::Repositioning;
			startTotemStage(0);
			playSoundEffect(Audio::SoundId::DevilAggro, 0.75f, true, 0.85f);
			playWalk();
		}
	}

	void RiftBinder::handleRepositioningState(const float dt)
	{
		Entity::update(dt);

		if (!hasValidTarget()) {
			_state = State::Idle;
			stopMoving();
			playIdle();
			return;
		}

		const float distance = getDistanceToTarget();
		if (distance > LEASH_RANGE) {
			_state = State::Idle;
			stopMoving();
			playIdle();
			return;
		}

		if (distance < MIN_DISTANCE && _blink_cooldown_timer <= 0.0f) {
			startDragonBlinkCast();
			return;
		}

		if (distance <= MELEE_RANGE && _melee_cooldown_timer <= 0.0f && !_shield_active) {
			startMeleeAttack();
			return;
		}

		if (_action_cooldown_timer <= 0.0f) {
			tryStartAction();
			return;
		}

		if (distance < MIN_DISTANCE) {
			moveAwayFromTarget(dt);
			return;
		}

		if (distance > PREFERRED_DISTANCE + 2.0f) {
			chaseToPreferredRange(dt);
			return;
		}

		stopMoving();
		playIdle();
	}

	void RiftBinder::handleCastingState(const float dt)
	{
		Entity::update(dt);
		stopMoving();

		if (const auto target = getTarget())
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);

		_cast_timer -= dt;
		if (_cast_timer <= 0.0f)
			finishSpellCast();
	}

	void RiftBinder::handleRecoveringState(const float dt)
	{
		Entity::update(dt);
		stopMoving();

		_recover_timer -= dt;
		if (_recover_timer > 0.0f)
			return;

		_state = State::Repositioning;
		playWalk();
	}

	void RiftBinder::handleMeleeAttackingState(const float dt)
	{
		Entity::update(dt);

		if (const auto target = getTarget())
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);

		const int frame_count = getAnimationFrameCount("attack");
		const float damage_frame = static_cast<float>(frame_count) * MELEE_DAMAGE_FRAME_RATIO;
		if (!_melee_damage_applied && frame_count > 0 && hasAnimationReachedFrame(damage_frame))
			applyMeleeDamage();

		if (!isAnimationLocked()) {
			if (!_melee_damage_applied)
				applyMeleeDamage();

			_melee_cooldown_timer = MELEE_COOLDOWN;
			_action_cooldown_timer = std::max(_action_cooldown_timer, 0.35f);
			_state = State::Repositioning;
			playWalk();
		}
	}

	void RiftBinder::handleHitReactingState(const float dt)
	{
		Entity::update(dt);
		stopMoving();

		_hit_react_timer -= dt;
		if (_hit_react_timer > 0.0f && isAnimationLocked())
			return;

		_state = hasValidTarget() ? State::Repositioning : State::Idle;
		if (_state == State::Repositioning)
			playWalk();
		else
			playIdle();
	}

	void RiftBinder::updateTotemStage()
	{
		if (!_shield_active)
			return;

		if (livingTotemCount() > 0)
			return;

		_shield_active = false;
		_active_stage = -1;
		_action_cooldown_timer = 0.15f;
		playSoundEffect(Audio::SoundId::DevilDashHit, 0.45f, true, 0.75f);
	}

	void RiftBinder::tryStartAction()
	{
		if (canCastFireRain() && _fire_rain_cooldown_timer <= 0.0f) {
			startFireRainCast();
			return;
		}

		if (_blink_cooldown_timer <= 0.0f && GetRandomValue(0, 99) < (_shield_active ? 6 : 3)) {
			startDragonBlinkCast();
			return;
		}

		if (_stone_cooldown_timer <= 0.0f) {
			startStoneVolleyCast();
			return;
		}

		if (_blink_cooldown_timer <= 0.0f) {
			startDragonBlinkCast();
			return;
		}

		_action_cooldown_timer = 0.18f;
	}

	void RiftBinder::startStoneVolleyCast()
	{
		startSpellCast(Spell::StoneVolley, "Stone Volley", "stone_volley", STONE_VOLLEY_CAST_TIME);
		_stone_cooldown_timer = STONE_COOLDOWN;
	}

	void RiftBinder::startFireRainCast()
	{
		if (!canCastFireRain())
			return;

		startSpellCast(Spell::FireRain, "Fire Rain", "fire_rain", FIRE_RAIN_CAST_TIME);
		spawnFireRain(FIRE_RAIN_CAST_TIME);
		_fire_rain_cooldown_timer = currentFireRainCooldown();
	}

	void RiftBinder::startDragonBlinkCast()
	{
		startSpellCast(Spell::DragonBlink, "Dragon Blink", "teleport", DRAGON_BLINK_CAST_TIME);
		_blink_cooldown_timer = BLINK_COOLDOWN;
	}

	void RiftBinder::startSpellCast(
		const Spell spell,
		const char* cast_name,
		const char* animation_name,
		const float cast_time)
	{
		_state = State::Casting;
		_casting_spell = spell;
		_cast_timer = cast_time;
		beginCastTelemetry(cast_name, cast_time, false);
		stopMoving();
		setAnimationSpeed(_shield_active ? 1.55f : 1.25f);
		playAnimation(animation_name, false, true, 0, true);
	}

	void RiftBinder::finishSpellCast()
	{
		switch (_casting_spell) {
			case Spell::StoneVolley:
				spawnStoneVolley();
				break;
			case Spell::DragonBlink:
				performRandomTeleport();
				break;
			case Spell::FireRain:
			case Spell::None:
				break;
		}

		clearCastTelemetry();
		_casting_spell = Spell::None;
		_action_cooldown_timer = actionCooldownDuration();
		_recover_timer = RECOVERY_TIME;
		_state = State::Recovering;
		playIdle();
	}

	void RiftBinder::startMeleeAttack()
	{
		_state = State::MeleeAttacking;
		_melee_damage_applied = false;
		stopMoving();
		setAnimationSpeed(MELEE_ANIMATION_SPEED);
		playAnimation("attack", false, true, 0, true);
	}

	void RiftBinder::startHitReact()
	{
		if (isDying() || isDead() || getAnimationFrameCount("get_hit") <= 0)
			return;

		if (_state == State::Casting || _state == State::MeleeAttacking)
			return;

		_state = State::HitReacting;
		_hit_react_timer = HIT_REACT_TIME;
		stopMoving();
		setAnimationSpeed(HIT_REACT_ANIMATION_SPEED);
		playAnimation("get_hit", false, true, 0, true);
	}

	void RiftBinder::startTotemStage(const int stage_index)
	{
		if (stage_index < 0 || stage_index >= static_cast<int>(TOTEMS_BY_STAGE.size()))
			return;

		_active_stage = stage_index;
		_next_stage_to_start = std::max(_next_stage_to_start, stage_index + 1);
		_shield_active = true;
		_totems.clear();
		spawnStageTotems(stage_index);
		_action_cooldown_timer = 0.20f;
		_stone_cooldown_timer = 0.20f;
		_fire_rain_cooldown_timer = std::min(_fire_rain_cooldown_timer, stage_index == 0 ? 2.0f : 0.9f);
		_blink_cooldown_timer = std::max(_blink_cooldown_timer, 8.0f);
		playSoundEffect(Audio::SoundId::DevilDash, 0.72f, true, 0.82f);
	}

	void RiftBinder::spawnStageTotems(const int stage_index)
	{
		std::shared_ptr<Entity> owner;
		try {
			owner = shared_from_this();
		} catch (const std::bad_weak_ptr&) {
			return;
		}

		const auto target = getTarget();
		const int count = TOTEMS_BY_STAGE[static_cast<size_t>(stage_index)];
		const Vector2 center = getCenter();
		const Vector2 target_pos = targetCenterOrSelf();
		const Vector2 forward = safeNormalize(Vector2Subtract(target_pos, center), {1.0f, 0.0f});
		const float base_angle = std::atan2(forward.y, forward.x) + randomFloat(-0.25f, 0.25f);
		const float angle_step = 2.0f * PI / static_cast<float>(count);
		std::vector<Vector2> spawned_positions;

		for (int i = 0; i < count; ++i) {
			Vector2 spawn_pos = findWalkableNearby(center, center);
			bool found_position = false;

			for (int attempt = 0; attempt < TOTEM_POSITION_ATTEMPTS; ++attempt) {
				const float angle = base_angle + static_cast<float>(i) * angle_step +
					randomFloat(-angle_step * 0.32f, angle_step * 0.32f);
				const float radius = TOTEM_RING_RADIUS +
					randomFloat(-TOTEM_RING_RADIUS_JITTER, TOTEM_RING_RADIUS_JITTER);
				const Vector2 direction = {std::cos(angle), std::sin(angle)};
				const Vector2 tangent = {-direction.y, direction.x};
				const float side_offset = randomFloat(-TOTEM_TANGENTIAL_JITTER, TOTEM_TANGENTIAL_JITTER);
				const Vector2 preferred = {
					center.x + direction.x * radius + tangent.x * side_offset,
					center.y + direction.y * radius + tangent.y * side_offset
				};
				const Vector2 candidate = findWalkableNearby(preferred, center);

				if (!isReachableWalkable(center, candidate))
					continue;

				const bool can_overlap = attempt >= TOTEM_POSITION_ATTEMPTS / 2;
				if (can_overlap || isFarEnoughFrom(spawned_positions, candidate, TOTEM_MIN_SEPARATION)) {
					spawn_pos = candidate;
					found_position = true;
					break;
				}
			}

			if (!found_position)
				spawn_pos = findWalkableNearby(center, center);

			spawned_positions.push_back(spawn_pos);
			auto totem = std::make_shared<RiftTotem>(
				spawn_pos.x,
				spawn_pos.y,
				_map,
				owner,
				target,
				stage_index);
			totem->setAltitude(getAltitude());
			totem->setAudioManager(getAudioManager());
			_totems.push_back(totem);
			addPendingSpawn(totem);
		}
	}

	void RiftBinder::spawnStoneVolley()
	{
		const auto target = getTarget();
		const Vector2 boss_pos = getCenter();
		const Vector2 target_pos = targetCenterOrSelf();
		const float target_height = target ? target->getAltitude() + 0.75f : getAltitude() + 0.75f;
		const Vector2 forward = safeNormalize(Vector2Subtract(target_pos, boss_pos), {1.0f, 0.0f});
		const Vector2 right = {-forward.y, forward.x};

		AbilityStats stats;
		stats.damage = static_cast<int>(std::round(static_cast<float>(STONE_DAMAGE) * getDamageMultiplier()));
		stats.duration = STONE_PROJECTILE_DURATION;
		stats.projectile_speed = STONE_PROJECTILE_SPEED;
		stats.hitbox_radius = STONE_PROJECTILE_HIT_RADIUS;

		for (int i = 0; i < STONE_PROJECTILE_COUNT; ++i) {
			const float side = i == 0 ? 0.0f : (i == 1 ? -1.0f : 1.0f);
			const Vector2 spawn_pos = {
				boss_pos.x + forward.x * STONE_PROJECTILE_SPAWN_FORWARD + right.x * side * STONE_PROJECTILE_SPAWN_SIDE,
				boss_pos.y + forward.y * STONE_PROJECTILE_SPAWN_FORWARD + right.y * side * STONE_PROJECTILE_SPAWN_SIDE
			};
			const Vector2 aim_pos = {
				target_pos.x + right.x * side * STONE_PROJECTILE_SPREAD,
				target_pos.y + right.y * side * STONE_PROJECTILE_SPREAD
			};

			auto projectile = std::make_shared<Projectile>(
				"Stone Shard",
				spawn_pos.x,
				spawn_pos.y,
				aim_pos.x,
				aim_pos.y,
				STONE_PROJECTILE_MODEL,
				STONE_PROJECTILE_MODEL_SCALE,
				stats,
				this,
				target_height);
			projectile->setFaction(Faction::Enemy);
			projectile->setAltitude(getAltitude());
			projectile->setModelTint(STONE_PROJECTILE_TINT);
			addPendingSpawn(projectile);
		}
	}

	void RiftBinder::spawnBlinkFlare(const Vector2 position)
	{
		BossTelegraphHazardConfig config;
		config.name = getName() + " Dragon Flare";
		config.position = findWalkableNearby(position, position);
		config.altitude = getAltitude();
		config.radius = BLINK_FLARE_RADIUS;
		config.warning_seconds = 0.0f;
		config.active_seconds = 0.26f;
		config.damage_per_tick = 0;
		config.tick_interval = 8.0f;
		config.source_context = Entity::makeDamageSourceContext(this, config.name);
		config.warning_color = {155, 80, 255, 255};
		config.active_color = {120, 35, 230, 255};
		addPendingSpawn(std::make_shared<BossTelegraphHazard>(std::move(config)));
	}

	void RiftBinder::spawnFireRain(const float warning_seconds)
	{
		BossTelegraphHazardConfig config;
		config.name = getName() + " Fire Rain";
		config.position = findWalkableNearby(targetCenterOrSelf(), getCenter());
		config.altitude = getAltitude();
		config.radius = FIRE_RAIN_RADIUS + (_shield_active ? 0.25f * static_cast<float>(std::max(0, _active_stage)) : 0.0f);
		config.warning_seconds = warning_seconds;
		config.active_seconds = FIRE_RAIN_ACTIVE_SECONDS;
		config.damage_per_tick = FIRE_RAIN_DAMAGE;
		config.tick_interval = FIRE_RAIN_TICK_INTERVAL;
		config.source_context = Entity::makeDamageSourceContext(this, config.name);
		config.warning_color = {255, 165, 45, 255};
		config.active_color = {245, 58, 18, 255};
		addPendingSpawn(std::make_shared<FireRainHazard>(std::move(config)));
	}

	void RiftBinder::performRandomTeleport()
	{
		const Vector2 old_pos = getCenter();
		const Vector2 destination = findTeleportDestination();
		setX(destination.x);
		setY(destination.y);
		spawnBlinkFlare(old_pos);
		spawnBlinkFlare(destination);
		playSoundEffect(Audio::SoundId::DevilDash, 0.58f, true, 1.12f);
	}

	void RiftBinder::applyMeleeDamage()
	{
		const auto target = getTarget();
		if (target && !target->isDead() && !target->isDying() && getDistanceToTarget() <= MELEE_RANGE * 1.55f) {
			target->takeDamage(
				static_cast<int>(static_cast<float>(MELEE_DAMAGE) * getDamageMultiplier()),
				Entity::makeDamageSourceContext(this, "Dragon Claw"));
			target->applyRoot(MELEE_ROOT_SECONDS);
			playSoundEffect(Audio::SoundId::DevilPunch, 0.72f, true, 0.9f);
		}

		_melee_damage_applied = true;
	}

	void RiftBinder::moveAwayFromTarget(const float dt)
	{
		const auto target = getTarget();
		if (!target)
			return;

		const Vector2 away = safeNormalize(Vector2Subtract(getCenter(), target->getCenter()), {1.0f, 0.0f});
		const Vector2 next = {
			getX() + away.x * MOVE_SPEED * getSpeedMultiplier() * 1.15f * dt,
			getY() + away.y * MOVE_SPEED * getSpeedMultiplier() * 1.15f * dt
		};

		if (!_map || _map->isWalkable(next.x, next.y)) {
			setPosition(next);
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);
			playWalk();
			return;
		}

		stopMoving();
		playIdle();
	}

	void RiftBinder::chaseToPreferredRange(const float dt)
	{
		const auto target = getTarget();
		if (!target)
			return;

		tickPathRecalcTimer(dt);
		if (isPathRecalcDue() || !isMoving()) {
			const Vector2 target_pos = target->getCenter();
			const Vector2 from_target = safeNormalize(Vector2Subtract(getCenter(), target_pos), {-1.0f, 0.0f});
			const Vector2 preferred = {
				target_pos.x + from_target.x * PREFERRED_DISTANCE,
				target_pos.y + from_target.y * PREFERRED_DISTANCE
			};
			const Vector2 walkable = findWalkableNearby(preferred, target_pos);
			moveTo(walkable.x, walkable.y);
			resetPathRecalcTimer(DEFAULT_PATH_RECALC_INTERVAL);
		}

		updateMovement(dt);
		if (isMoving())
			playWalk();
		else
			playIdle();
	}

	void RiftBinder::stopMoving()
	{
		setVelocity(0.0f, 0.0f);
		stopMovement();
	}

	void RiftBinder::playIdle()
	{
		setAnimationSpeed(1.0f);
		playAnimation("idle");
	}

	void RiftBinder::playWalk()
	{
		setAnimationSpeed(_shield_active ? 1.22f : 1.0f);
		playAnimation("walk");
	}

	void RiftBinder::onDeathStarted()
	{
		clearCastTelemetry();
		stopMoving();
		playSoundEffect(Audio::SoundId::DevilDeath, 0.9f, true, 0.82f);
	}

	Vector2 RiftBinder::findWalkableNearby(const Vector2 preferred, const Vector2 fallback) const
	{
		if (!_map)
			return preferred;

		if (isReachableWalkable(fallback, preferred))
			return preferred;

		for (const float radius : {0.7f, 1.4f, 2.2f, 3.0f, 4.0f}) {
			const float angle_offset = randomFloat(0.0f, 360.0f) * DEG2RAD;
			for (int i = 0; i < 12; ++i) {
				const float angle = angle_offset + (static_cast<float>(i) / 12.0f) * 2.0f * PI;
				const Vector2 candidate = {
					preferred.x + std::cos(angle) * radius,
					preferred.y + std::sin(angle) * radius
				};

				if (isReachableWalkable(fallback, candidate))
					return candidate;
			}
		}

		if (_map->getNavMesh().isReady()) {
			const Vector3 snapped = _map->getNavMesh().getClosestWalkablePosition({preferred.x, getAltitude(), preferred.y});
			const Vector2 snapped_position = {snapped.x, snapped.z};
			if (isReachableWalkable(fallback, snapped_position))
				return snapped_position;
		}

		if (isReachableWalkable(fallback, fallback))
			return fallback;

		return fallback;
	}

	bool RiftBinder::isReachableWalkable(const Vector2 from, const Vector2 position) const
	{
		if (!_map)
			return true;

		if (!_map->isWalkable(position.x, position.y))
			return false;

		if (!_map->getNavMesh().isReady())
			return true;

		const auto path = _map->findPath(
			{from.x, getAltitude(), from.y},
			{position.x, getAltitude(), position.y});
		return !path.empty();
	}

	Vector2 RiftBinder::findTeleportDestination() const
	{
		const Vector2 center = targetCenterOrSelf();
		const Vector2 fallback = findWalkableNearby(getCenter(), getCenter());

		for (int i = 0; i < 18; ++i) {
			const float angle = randomFloat(0.0f, 360.0f) * DEG2RAD;
			const float radius = randomFloat(BLINK_MIN_RADIUS, BLINK_MAX_RADIUS);
			const Vector2 candidate = {
				center.x + std::cos(angle) * radius,
				center.y + std::sin(angle) * radius
			};
			const Vector2 walkable = findWalkableNearby(candidate, fallback);
			if (Vector2DistanceSqr(walkable, center) >= MIN_DISTANCE * MIN_DISTANCE)
				return walkable;
		}

		return fallback;
	}

	Vector2 RiftBinder::targetCenterOrSelf() const
	{
		if (const auto target = getTarget())
			return target->getCenter();

		return getCenter();
	}

	int RiftBinder::livingTotemCount()
	{
		return static_cast<int>(liveTotems().size());
	}

	std::vector<std::shared_ptr<RiftTotem>> RiftBinder::liveTotems()
	{
		std::vector<std::shared_ptr<RiftTotem>> result;
		std::erase_if(_totems, [&result](const std::weak_ptr<RiftTotem>& weak_totem) {
			const auto totem = weak_totem.lock();
			if (!totem || totem->isDead() || totem->isDying())
				return true;

			result.push_back(totem);
			return false;
		});

		return result;
	}

	float RiftBinder::actionCooldownDuration() const
	{
		return _shield_active ? SHIELDED_ACTION_COOLDOWN : ACTION_COOLDOWN;
	}

	bool RiftBinder::canCastFireRain()
	{
		return _shield_active &&
			   _active_stage >= 0 &&
			   _active_stage < static_cast<int>(TOTEMS_BY_STAGE.size()) &&
			   livingTotemCount() > 0;
	}

	float RiftBinder::currentFireRainCooldown()
	{
		if (_active_stage < 0 || _active_stage >= static_cast<int>(TOTEMS_BY_STAGE.size()))
			return FIRE_RAIN_STAGE_BASE_COOLDOWNS.front();

		const int total_totems = TOTEMS_BY_STAGE[static_cast<size_t>(_active_stage)];
		const int living_totems = std::clamp(livingTotemCount(), 0, total_totems);
		const int destroyed_totems = std::clamp(total_totems - living_totems, 0, total_totems);
		const float base_cooldown = FIRE_RAIN_STAGE_BASE_COOLDOWNS[static_cast<size_t>(_active_stage)];
		const float min_cooldown = FIRE_RAIN_STAGE_MIN_COOLDOWNS[static_cast<size_t>(_active_stage)];
		const float step = total_totems > 0
			? (base_cooldown - min_cooldown) / static_cast<float>(total_totems)
			: 0.0f;

		return std::max(min_cooldown, base_cooldown - step * static_cast<float>(destroyed_totems));
	}

} // namespace Nawia::Entity
