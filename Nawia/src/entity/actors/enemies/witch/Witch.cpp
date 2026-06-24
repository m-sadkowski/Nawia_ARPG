#include "Witch.h"

#include <Collider.h>
#include <Logger.h>
#include <Map.h>
#include <Player.h>
#include <Projectile.h>
#include <ResourceManager.h>
#include <SoundIds.h>
#include <WalkingDead.h>

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

	namespace {
		constexpr const char* WITCH_MODEL = "assets/models/actors/witch/witch.glb";
		constexpr const char* FIREBALL_MODEL = "assets/models/fireball.glb";
		constexpr int ANIM_DEATH = 0;
		constexpr int ANIM_HIT = 2;
		constexpr int ANIM_IDLE = 5;
		constexpr int ANIM_BOLT = 7;
		constexpr int ANIM_SUMMON = 10;
		constexpr int ANIM_RUN = 16;

		AbilityStats makeWitchBoltStats() {
			AbilityStats stats;
			stats.damage = 18;
			stats.duration = 2.0f;
			stats.projectile_speed = 10.0f;
			stats.hitbox_radius = 1.0f;
			return stats;
		}
	}

	Witch::Witch() {
		configureModel();
		setFaction(Faction::Enemy);
		setPersistAfterDeath(true);
		setMovementSpeed(MOVE_SPEED);
	}

	Witch::Witch(const float x, const float y, Core::Map* map)
		: EnemyInterface("Czarownica", x, y, nullptr, 160, map)
	{
		configureModel();
		setFaction(Faction::Enemy);
		setPersistAfterDeath(true);
		setMovementSpeed(MOVE_SPEED);
		setCollider(std::make_unique<RectangleCollider>(this, 1.0f, 1.25f, 0.0f, 0.0f));
	}

	void Witch::configureModel() {
		setScale(MODEL_SCALE);
		setModelFacingOffset(90.0f);
		loadModel(WITCH_MODEL);
		loadAnimationBundle(WITCH_MODEL);
		addAnimation("death", WITCH_MODEL, ANIM_DEATH);
		addAnimation("get_hit", WITCH_MODEL, ANIM_HIT);
		addAnimation("idle", WITCH_MODEL, ANIM_IDLE);
		addAnimation("bolt", WITCH_MODEL, ANIM_BOLT);
		addAnimation("summon", WITCH_MODEL, ANIM_SUMMON);
		addAnimation("run", WITCH_MODEL, ANIM_RUN);
		playIdle();
	}

	void Witch::takeDamage(const int dmg) {
		Entity::takeDamage(dmg);
		if (isDying())
			return;

		if (GetRandomValue(0, 99) < 50) {
			startImmediateSummonRetaliation();
			return;
		}

		_state = State::GettingHit;
		_cast_projectile_spawned = false;
		_retaliation_applied = false;
		stopMoving();
		setAnimationSpeed(1.0f);
		playAnimation("get_hit", false, true, 0, true);
	}

	void Witch::update(const float dt) {
		if (isDying()) {
			updateDeathFreeze(dt);
			return;
		}

		if (isDormant())
			return;

		if (_cast_cooldown_timer > 0.0f)
			_cast_cooldown_timer -= dt;

		switch (_state) {
			case State::Idle:
				handleIdleState(dt);
				break;
			case State::Repositioning:
				handleRepositioningState(dt);
				break;
			case State::CastingBolt:
				handleCastingBoltState(dt);
				break;
			case State::GettingHit:
				handleGettingHitState(dt);
				break;
			case State::Retaliating:
				handleRetaliatingState(dt);
				break;
			case State::Summoning:
				handleSummoningState(dt);
				break;
		}
	}

	void Witch::handleIdleState(const float dt) {
		Entity::update(dt);

		const auto target = _target.lock();
		if (!target || target->isDead())
			return;

		const float dist = getDistanceToTarget();
		if (dist > VISION_RANGE)
			return;

		if (dist < MIN_DISTANCE || dist > CAST_RANGE) {
			_state = State::Repositioning;
			playRun();
			return;
		}

		if (_cast_cooldown_timer <= 0.0f)
			startBoltCast();
	}

	void Witch::handleRepositioningState(const float dt) {
		Entity::update(dt);

		const auto target = _target.lock();
		if (!target || target->isDead()) {
			stopMoving();
			_state = State::Idle;
			playIdle();
			return;
		}

		const float dist = getDistanceToTarget();
		if (dist < MIN_DISTANCE) {
			moveAwayFromTarget(dt);
			return;
		}

		if (dist > CAST_RANGE) {
			chaseToCastRange(dt);
			return;
		}

		stopMoving();
		_state = State::Idle;
		playIdle();
	}

	void Witch::handleCastingBoltState(const float dt) {
		Entity::update(dt);

		if (const auto target = _target.lock())
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);

		const int frame_count = getAnimationFrameCount("bolt");
		const float cast_frame = static_cast<float>(frame_count) * CAST_FRAME_RATIO;
		if (!_cast_projectile_spawned && frame_count > 0 && _anim_frame_counter >= cast_frame) {
			fireBolt();
			_cast_projectile_spawned = true;
		}

		if (!isAnimationLocked()) {
			_cast_cooldown_timer = CAST_COOLDOWN;
			_state = State::Idle;
			playIdle();
		}
	}

	void Witch::handleGettingHitState(const float dt) {
		Entity::update(dt);
		if (!isAnimationLocked())
			startRetaliation();
	}

	void Witch::handleRetaliatingState(const float dt) {
		Entity::update(dt);

		if (const auto target = _target.lock())
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);

		const int frame_count = getAnimationFrameCount("bolt");
		const float effect_frame = static_cast<float>(frame_count) * CAST_FRAME_RATIO;
		if (!_retaliation_applied && frame_count > 0 && _anim_frame_counter >= effect_frame) {
			applyRetaliation();
			_retaliation_applied = true;
		}

		if (!isAnimationLocked()) {
			_cast_cooldown_timer = CAST_COOLDOWN * 0.65f;
			_state = State::Idle;
			playIdle();
		}
	}

	void Witch::handleSummoningState(const float dt) {
		Entity::update(dt);

		if (const auto target = _target.lock())
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);

		const int frame_count = getAnimationFrameCount("summon");
		const float effect_frame = static_cast<float>(frame_count) * CAST_FRAME_RATIO;
		if (!_retaliation_applied && frame_count > 0 && _anim_frame_counter >= effect_frame) {
			summonHelper();
			_retaliation_applied = true;
		}

		if (!isAnimationLocked()) {
			_cast_cooldown_timer = CAST_COOLDOWN * 0.75f;
			_state = State::Idle;
			playIdle();
		}
	}

	void Witch::startBoltCast() {
		_state = State::CastingBolt;
		_cast_projectile_spawned = false;
		stopMoving();
		setAnimationSpeed(1.0f);
		playAnimation("bolt", false, true, 0, true);
	}

	void Witch::fireBolt() {
		const auto target = _target.lock();
		if (!target)
			return;

		const Model* shared_model = nullptr;
		if (auto* resource_manager = Entity::getSharedResourceManager())
			shared_model = resource_manager->getModel(FIREBALL_MODEL);

		const Vector2 target_pos = target->getCenter();
		const float target_height = target->getAltitude() + 1.0f;
		playSoundEffect(Audio::SoundId::FireballCast, 0.75f, true, 0.82f);
		auto projectile = std::make_shared<Projectile>(
			"Fireball",
			getCenter().x,
			getCenter().y,
			target_pos.x,
			target_pos.y,
			FIREBALL_MODEL,
			0.45f,
			makeWitchBoltStats(),
			this,
			target_height,
			nullptr,
			0.0f,
			shared_model);
		projectile->setFaction(Faction::Enemy);
		addPendingSpawn(projectile);
	}

	void Witch::startRetaliation() {
		_state = State::Retaliating;
		_retaliation_applied = false;
		stopMoving();
		setAnimationSpeed(1.0f);
		playAnimation("bolt", false, true, 0, true);
	}

	void Witch::startImmediateSummonRetaliation() {
		_state = State::Summoning;
		_retaliation_applied = false;
		_cast_projectile_spawned = false;
		stopMoving();

		if (const auto target = std::dynamic_pointer_cast<Player>(_target.lock())) {
			target->rememberDamageSource(this);
			target->knockDown(static_cast<int>(RETALIATION_DAMAGE * _damage_multiplier));
		}

		setAnimationSpeed(1.0f);
		playAnimation("summon", false, true, 0, true);
	}

	void Witch::applyRetaliation() {
		const auto target = std::dynamic_pointer_cast<Player>(_target.lock());
		if (target) {
			target->rememberDamageSource(this);
			target->knockDown(static_cast<int>(RETALIATION_DAMAGE * _damage_multiplier));
		}

		summonHelper();
	}

	void Witch::summonHelper() {
		auto target = _target.lock();
		Vector2 spawn_pos = getCenter();
		if (target) {
			const Vector2 dir = Vector2Normalize(Vector2Subtract(getCenter(), target->getCenter()));
			spawn_pos.x += dir.x * 2.0f;
			spawn_pos.y += dir.y * 2.0f;
		}

		auto helper = std::shared_ptr<Entity>(WalkingDeadBuilder()
			.setName("Pomagier Czarownicy")
			.setPosition(spawn_pos)
			.setMap(_map)
			.setMaxHp(55)
			.setTarget(target)
			.setAudioManager(_audio_manager)
			.build());

		helper->setAltitude(getAltitude());
		addPendingSpawn(helper);
	}

	void Witch::moveAwayFromTarget(const float dt) {
		const auto target = _target.lock();
		if (!target)
			return;

		const Vector2 away = Vector2Normalize(Vector2Subtract(getCenter(), target->getCenter()));
		const Vector2 next = {
			getX() + away.x * MOVE_SPEED * dt,
			getY() + away.y * MOVE_SPEED * dt
		};

		if (!_map || _map->isWalkable(next.x, next.y)) {
			_pos = next;
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);
			playRun();
			return;
		}

		stopMoving();
	}

	void Witch::chaseToCastRange(const float dt) {
		const auto target = _target.lock();
		if (!target)
			return;

		_path_recalc_timer -= dt;
		if (_path_recalc_timer <= 0.0f || !_is_moving) {
			const Vector2 my_pos = getCenter();
			const Vector2 target_pos = target->getCenter();
			const Vector2 toward = Vector2Normalize(Vector2Subtract(target_pos, my_pos));
			const Vector2 desired = {
				target_pos.x - toward.x * PREFERRED_DISTANCE,
				target_pos.y - toward.y * PREFERRED_DISTANCE
			};
			moveTo(desired.x, desired.y);
			_path_recalc_timer = DEFAULT_PATH_RECALC_INTERVAL;
		}

		updateMovement(dt);
		if (_is_moving)
			playRun();
	}

	void Witch::stopMoving() {
		setVelocity(0.0f, 0.0f);
		_is_moving = false;
	}

	void Witch::updateDeathFreeze(const float dt) {
		const int frame_count = getAnimationFrameCount("death");
		if (frame_count <= 0) {
			die();
			return;
		}

		const float final_frame = static_cast<float>(std::max(0, frame_count - 1));
		_anim_frame_counter += dt * _anim_fps * _anim_speed_multiplier / ANIMATION_DURATION_SCALE;
		if (_anim_frame_counter >= final_frame) {
			_anim_frame_counter = final_frame;
			_anim_locked = true;
			_hp = 0;
		}

		applyCurrentAnimationFrame();
	}

	void Witch::playIdle() {
		setAnimationSpeed(1.0f);
		playAnimation("idle", true, false, 0, true);
	}

	void Witch::playRun() {
		setAnimationSpeed(1.0f);
		playAnimation("run");
	}

	void Witch::onDeathStarted() {
		stopMoving();
	}

} // namespace Nawia::Entity
