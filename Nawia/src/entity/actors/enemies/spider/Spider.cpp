#include "Spider.h"

#include <CobwebProjectile.h>
#include <Collider.h>
#include <Map.h>
#include <ResourceManager.h>
#include <SoundIds.h>

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

	namespace {
		constexpr const char* SPIDER_MODEL = "assets/models/actors/spider/spider.glb";
		constexpr const char* COBWEB_MODEL = "assets/models/cobweb.glb";
		constexpr int ANIM_MELEE = 0;
		constexpr int ANIM_DEATH = 1;
		constexpr int ANIM_IDLE = 2;
		constexpr int ANIM_WEB = 3;
		constexpr int ANIM_WALK = 4;
		constexpr float NAV_POINT_REACHED_DISTANCE_SQ = 0.16f;
		constexpr float NAV_TARGET_CHANGE_DISTANCE_SQ = 0.64f;
		constexpr float WEB_BACK_OFFSET = 0.95f;

		void makeSpiderMaterialsVisible(Model& model)
		{
			for (int i = 0; i < model.materialCount; ++i) {
				if (!model.materials[i].maps)
					continue;

				Color& color = model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color;
				color.a = 255;

				const int strongest_channel = std::max({static_cast<int>(color.r), static_cast<int>(color.g), static_cast<int>(color.b)});
				if (strongest_channel <= 0) {
					color = Color{64, 58, 62, 255};
					continue;
				}

				if (strongest_channel < 72) {
					const float multiplier = 72.0f / static_cast<float>(strongest_channel);
					color.r = static_cast<unsigned char>(std::clamp(static_cast<int>(std::round(static_cast<float>(color.r) * multiplier)), 0, 255));
					color.g = static_cast<unsigned char>(std::clamp(static_cast<int>(std::round(static_cast<float>(color.g) * multiplier)), 0, 255));
					color.b = static_cast<unsigned char>(std::clamp(static_cast<int>(std::round(static_cast<float>(color.b) * multiplier)), 0, 255));
				}
			}
		}
	}

	Spider::Spider()
	{
		setName("Pajak");
		setMaxHp(240);
		setHealToFullOnKill(true);
		setFaction(Faction::Enemy);
		setMovementSpeed(MOVE_SPEED);
		setCollider(std::make_unique<RectangleCollider>(this, 1.6f, 1.35f, 0.0f, 0.0f));
		setDeathAnimationName("death");
		configureModel();
	}

	void Spider::configureModel()
	{
		setScale(MODEL_SCALE);
		setModelFacingOffset(90.0f);
		loadModel(SPIDER_MODEL);
		if (hasModelLoaded())
			makeSpiderMaterialsVisible(getModel());
		loadAnimationBundle(SPIDER_MODEL);
		addAnimation("melee", SPIDER_MODEL, ANIM_MELEE);
		addAnimation("death", SPIDER_MODEL, ANIM_DEATH);
		addAnimation("idle", SPIDER_MODEL, ANIM_IDLE);
		addAnimation("web", SPIDER_MODEL, ANIM_WEB);
		addAnimation("walk", SPIDER_MODEL, ANIM_WALK);
		playIdle();
	}

	void Spider::takeDamage(const int dmg)
	{
		Entity::takeDamage(dmg);
		if (isDying())
			return;

		if (_state != State::GettingHit)
			_state_before_hit = _state;

		_state = State::GettingHit;
		stopMoving();
	}

	void Spider::update(const float dt)
	{
		if (isDying()) {
			updateMovementSound(Audio::SoundPath::SpiderWalk, false);
			Entity::update(dt);
			return;
		}

		if (isDormant())
			return;

		if (_melee_cooldown_timer > 0.0f)
			_melee_cooldown_timer -= dt;
		if (_web_cooldown_timer > 0.0f)
			_web_cooldown_timer -= dt;

		switch (_state) {
			case State::Idle:
				handleIdleState(dt);
				break;
			case State::Chasing:
				handleChasingState(dt);
				break;
			case State::MeleeAttacking:
				handleMeleeAttackingState(dt);
				break;
			case State::WebAttacking:
				handleWebAttackingState(dt);
				break;
			case State::GettingHit:
				handleGettingHitState(dt);
				break;
		}
	}

	void Spider::handleIdleState(const float dt)
	{
		Entity::update(dt);

		if (hasValidTarget() && getDistanceToTarget() <= VISION_RANGE) {
			_state = State::Chasing;
			playWalk();
		}
	}

	void Spider::handleChasingState(const float dt)
	{
		Entity::update(dt);

		if (!hasValidTarget()) {
			_state = State::Idle;
			clearNavigationPath();
			stopMoving();
			playIdle();
			return;
		}

		const auto target = _target.lock();
		const float distance = getDistanceToTarget();
		if (distance > VISION_RANGE * 1.7f) {
			_state = State::Idle;
			clearNavigationPath();
			stopMoving();
			playIdle();
			return;
		}

		const Vector2 target_pos = target->getCenter();
		const bool target_webbed = target->isMovementRooted();
		const float chase_speed_multiplier = target_webbed ? WEB_CHASE_SPEED_MULTIPLIER : 1.0f;

		if (distance <= MELEE_RANGE && _melee_cooldown_timer <= 0.0f && canReachPositionWithNav(target_pos)) {
			startMeleeAttack();
			return;
		}

		if (!target_webbed &&
			distance <= WEB_RANGE &&
			distance >= WEB_MIN_RANGE &&
			_web_cooldown_timer <= 0.0f) {
			startWebAttack();
			return;
		}

		setMovementSpeed(MOVE_SPEED * chase_speed_multiplier);
		moveTowardPositionWithNav(target_pos, dt);
		rotateTowardsCenter(target_pos.x, target_pos.y);

		if (isMoving()) {
			playWalk(chase_speed_multiplier);
			updateMovementSound(Audio::SoundPath::SpiderWalk, true, 0.48f, target_webbed ? 1.15f : 0.9f);
		} else {
			playIdle();
			updateMovementSound(Audio::SoundPath::SpiderWalk, false);
		}
	}

	void Spider::handleMeleeAttackingState(const float dt)
	{
		Entity::update(dt);

		const auto target = _target.lock();
		if (target)
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);

		const int frame_count = getAnimationFrameCount("melee");
		const float damage_frame = static_cast<float>(frame_count) * MELEE_DAMAGE_FRAME_RATIO;
		if (!_melee_damage_applied && frame_count > 0 && _anim_frame_counter >= damage_frame) {
			if (target && !target->isDead() && !target->isDying() && getDistanceToTarget() <= MELEE_RANGE * 1.8f) {
				target->takeDamage(
					static_cast<int>(MELEE_DAMAGE * _damage_multiplier),
					Entity::makeDamageSourceContext(this, "Spider Bite"));
				target->applyPoison(
					POISON_DURATION,
					POISON_TICK_DAMAGE,
					1.0f,
					Entity::makeDamageSourceContext(this, "Spider Poison"));
				playSoundEffect(Audio::SoundId::SpiderMeleeAttack, 0.75f, true, 1.0f);
			}
			_melee_damage_applied = true;
		}

		if (!isAnimationLocked()) {
			_melee_cooldown_timer = MELEE_COOLDOWN;
			_state = State::Chasing;
			playWalk();
		}
	}

	void Spider::handleWebAttackingState(const float dt)
	{
		Entity::update(dt);

		if (const auto target = _target.lock())
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);

		const int frame_count = getAnimationFrameCount("web");
		const float fire_frame = static_cast<float>(frame_count) * WEB_FIRE_FRAME_RATIO;
		if (!_web_fired && frame_count > 0 && _anim_frame_counter >= fire_frame) {
			fireWeb();
			_web_fired = true;
		}

		if (!isAnimationLocked()) {
			if (!_web_fired)
				fireWeb();

			_web_cooldown_timer = WEB_COOLDOWN;
			_state = State::Chasing;
			playWalk();
		}
	}

	void Spider::handleGettingHitState(const float dt)
	{
		Entity::update(dt);
		_state = _state_before_hit == State::Idle ? State::Idle : State::Chasing;
		if (_state == State::Idle)
			playIdle();
		else
			playWalk();
	}

	void Spider::startMeleeAttack()
	{
		_state = State::MeleeAttacking;
		_melee_damage_applied = false;
		clearNavigationPath();
		stopMoving();
		setAnimationSpeed(MELEE_ANIMATION_SPEED);
		playAnimation("melee", false, true, 0, true);
	}

	void Spider::startWebAttack()
	{
		_state = State::WebAttacking;
		_web_fired = false;
		clearNavigationPath();
		stopMoving();
		setAnimationSpeed(WEB_ANIMATION_SPEED);
		playAnimation("web", false, true, 0, true);
	}

	void Spider::fireWeb()
	{
		const auto target = _target.lock();
		if (!target || target->isDead() || target->isDying())
			return;

		const Vector2 spider_pos = getCenter();
		const Vector2 target_pos = target->getCenter();
		Vector2 direction = Vector2Subtract(target_pos, spider_pos);
		if (Vector2LengthSqr(direction) <= 0.001f)
			direction = {1.0f, 0.0f};
		else
			direction = Vector2Normalize(direction);

		const Vector2 spawn_pos = {
			spider_pos.x - direction.x * WEB_BACK_OFFSET,
			spider_pos.y - direction.y * WEB_BACK_OFFSET
		};

		const BoundingBox spider_box = getBoundingBox();
		const BoundingBox target_box = target->getBoundingBox();
		const float start_height = (spider_box.min.y + spider_box.max.y) * 0.5f;
		const float target_height = (target_box.min.y + target_box.max.y) * 0.5f;

		const Model* shared_model = nullptr;
		if (auto* resource_manager = Entity::getSharedResourceManager())
			shared_model = resource_manager->getModel(COBWEB_MODEL);

		playSoundEffect(Audio::SoundId::SpiderWebShot, 0.7f, true, 1.0f);
		addPendingSpawn(std::make_shared<CobwebProjectile>(
			spawn_pos.x,
			spawn_pos.y,
			target_pos.x,
			target_pos.y,
			start_height,
			target_height,
			shared_model,
			this));
	}

	void Spider::stopMoving()
	{
		setVelocity(0.0f, 0.0f);
		stopMovement();
		updateMovementSound(Audio::SoundPath::SpiderWalk, false);
	}

	void Spider::playIdle()
	{
		setMovementSpeed(MOVE_SPEED);
		setAnimationSpeed(1.0f);
		playAnimation("idle");
	}

	void Spider::playWalk(const float speed_multiplier)
	{
		setAnimationSpeed(std::max(0.1f, speed_multiplier));
		playAnimation("walk");
	}

	bool Spider::moveTowardPositionWithNav(const Vector2 target_pos, const float dt, const float repath_interval)
	{
		if (!_map || !_map->getNavMesh().isReady()) {
			clearNavigationPath();
			moveTo(target_pos.x, target_pos.y);
			updateMovement(dt);
			return isMoving();
		}

		tickPathRecalcTimer(dt);
		const bool target_changed = !_has_current_nav_target ||
			Vector2DistanceSqr(_current_nav_target, target_pos) > NAV_TARGET_CHANGE_DISTANCE_SQ;

		if (isPathRecalcDue() || target_changed) {
			_current_nav_path = _map->findPath(getWorldPos3D(), {target_pos.x, getAltitude(), target_pos.y});
			_current_nav_target = target_pos;
			_has_current_nav_target = true;
			resetPathRecalcTimer(repath_interval);
		}

		const Vector2 current_pos = getCenter();
		while (!_current_nav_path.empty() &&
			   Vector2DistanceSqr(current_pos, _current_nav_path.front()) <= NAV_POINT_REACHED_DISTANCE_SQ) {
			_current_nav_path.erase(_current_nav_path.begin());
		}

		if (_current_nav_path.empty()) {
			stopMoving();
			return false;
		}

		moveTo(_current_nav_path.front().x, _current_nav_path.front().y);
		updateMovement(dt);
		return isMoving();
	}

	bool Spider::canReachPositionWithNav(const Vector2 target_pos) const
	{
		if (!_map || !_map->getNavMesh().isReady())
			return true;

		return !_map->findPath(getWorldPos3D(), {target_pos.x, getAltitude(), target_pos.y}).empty();
	}

	void Spider::clearNavigationPath()
	{
		_current_nav_path.clear();
		_has_current_nav_target = false;
		clearPathRecalcTimer();
	}

	void Spider::onDeathStarted()
	{
		stopMoving();
		playSoundEffect(Audio::SoundId::ZombieDeath, 0.75f, true, 0.65f);
	}

} // namespace Nawia::Entity
