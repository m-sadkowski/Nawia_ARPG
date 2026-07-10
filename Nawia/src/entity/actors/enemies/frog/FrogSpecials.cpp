#include "Frog.h"

#include "FrogInternal.h"

#include <BossTelegraphHazard.h>
#include <Map.h>
#include <SoundIds.h>

#include <raymath.h>

#include <algorithm>
#include <memory>
#include <utility>

namespace Nawia::Entity {

	void Frog::updateSpecialState(const float dt) {
		switch (_special_state) {
			case SpecialState::TongueWindup:
				updateTongueWindup(dt);
				break;
			case SpecialState::TonguePull:
				updateTonguePull(dt);
				break;
			case SpecialState::TongueRecover:
				updateTongueRecover(dt);
				break;
			case SpecialState::SideHop:
				updateSideHop(dt);
				break;
			case SpecialState::ToxicSpitWindup:
				updateToxicSpitWindup(dt);
				break;
			case SpecialState::None:
				break;
		}
	}

	void Frog::updateTongueWindup(const float dt) {
		Entity::update(dt);
		stopMoving();
		rotateTowardsCenter(_tongue_target_snapshot.x, _tongue_target_snapshot.y);

		_special_timer -= dt;
		if (_special_timer <= 0.0f)
			releaseTongueStrike();
	}

	void Frog::updateTonguePull(const float dt) {
		Entity::update(dt);
		stopMoving();

		const auto victim = _tongue_victim.lock();
		if (!victim || victim->isDead()) {
			_special_state = SpecialState::TongueRecover;
			_special_timer = FrogTuning::TONGUE_RECOVER_TIME;
			return;
		}

		const Vector2 frog_center = getCenter();
		const Vector2 victim_center = victim->getCenter();
		const Vector2 toward_frog = Vector2Normalize(Vector2Subtract(frog_center, victim_center));
		const float distance = Vector2Distance(frog_center, victim_center);
		if (distance > FrogTuning::TONGUE_STOP_DISTANCE) {
			const float move_amount = std::min(FrogTuning::TONGUE_PULL_SPEED * dt, distance - FrogTuning::TONGUE_STOP_DISTANCE);
			const Vector2 next_center = {
				victim_center.x + toward_frog.x * move_amount,
				victim_center.y + toward_frog.y * move_amount
			};

			if (!_map || _map->isWalkable(next_center.x, next_center.y)) {
				victim->setX(victim->getX() + toward_frog.x * move_amount);
				victim->setY(victim->getY() + toward_frog.y * move_amount);
			}
		}

		victim->applyRoot(0.12f);
		rotateTowardsCenter(victim->getCenter().x, victim->getCenter().y);
		_special_timer -= dt;
		if (_special_timer <= 0.0f) {
			_tongue_victim.reset();
			_special_state = SpecialState::TongueRecover;
			_special_timer = FrogTuning::TONGUE_RECOVER_TIME;
		}
	}

	void Frog::updateTongueRecover(const float dt) {
		Entity::update(dt);
		stopMoving();

		_special_timer -= dt;
		if (_special_timer <= 0.0f)
			finishSpecialMove();
	}

	void Frog::updateSideHop(const float dt) {
		Entity::update(dt);

		_special_timer -= dt;
		setSpeedMultiplier(1.9f);
		moveTowardPositionWithNav(_sidehop_target, dt, 0.18f);
		if (isMoving()) {
			setAnimationSpeed(1.25f);
			playAnimation("walk");
		}

		faceTargetCenter();

		updateMovementSound(Audio::SoundPath::DevilStep, isMoving() && !isDormant(), 0.28f, 1.45f);
		if (_special_timer <= 0.0f || !isMoving())
			finishSpecialMove();
	}

	void Frog::updateToxicSpitWindup(const float dt) {
		Entity::update(dt);
		stopMoving();
		rotateTowardsCenter(_toxic_pool_target_snapshot.x, _toxic_pool_target_snapshot.y);

		_special_timer -= dt;
		if (_special_timer <= 0.0f)
			finishSpecialMove();
	}

	void Frog::tryStartSpecialMove() {
		if (!hasValidTarget() || isAnimationLocked() || _state == State::GettingHit)
			return;

		const float distance = getDistanceToTarget();
		if (isBossVariant() &&
			_toxic_pool_cooldown_timer <= 0.0f &&
			distance >= 3.0f &&
			distance <= 15.0f &&
			GetRandomValue(0, 99) < 58) {
			startToxicPool();
			return;
		}

		if (_tongue_cooldown_timer <= 0.0f &&
			distance >= FrogTuning::TONGUE_MIN_RANGE &&
			distance <= FrogTuning::TONGUE_MAX_RANGE &&
			canReachPositionWithNav(getTargetPosition()) &&
			GetRandomValue(0, 99) < 72) {
			startTongueStrike();
			return;
		}

		if (_sidehop_cooldown_timer <= 0.0f &&
			distance >= 3.5f &&
			distance <= 10.5f &&
			GetRandomValue(0, 99) < 45) {
			startSideHop();
		}
	}

	void Frog::startTongueStrike() {
		const auto target = getTarget();
		if (!target)
			return;

		_tongue_target_snapshot = target->getCenter();
		_tongue_victim.reset();
		_special_state = SpecialState::TongueWindup;
		_special_timer = FrogTuning::TONGUE_WINDUP_TIME;
		_tongue_cooldown_timer = randomRange(3.6f, 5.4f);
		_sidehop_cooldown_timer = std::max(_sidehop_cooldown_timer, 0.8f);
		_state = State::Chasing;
		stopMoving();
		clearNavigationPath();
		rotateTowardsCenter(_tongue_target_snapshot.x, _tongue_target_snapshot.y);
		beginCastTelemetry("Tongue Strike", FrogTuning::TONGUE_WINDUP_TIME, true);
		setAnimationSpeed(0.72f);
		playAnimation("attack", false, true, 0, true);
		playSoundEffect(Audio::SoundId::FrogSound, 0.78f, true, 1.08f);
	}

	void Frog::releaseTongueStrike() {
		clearCastTelemetry();
		const auto target = getTarget();
		if (target && isTargetInTongueLane(*target)) {
			target->rememberDamageSource(this, "Tongue Strike");
			target->takeDamage(static_cast<int>(FrogTuning::TONGUE_DAMAGE * getDamageMultiplier()));
			target->applyRoot(FrogTuning::TONGUE_PULL_TIME + 0.18f);
			_tongue_victim = target;
			playSoundEffect(Audio::SoundId::DevilDashHit, 0.72f, true, 1.25f);
		} else {
			_tongue_victim.reset();
			playSoundEffect(Audio::SoundId::DevilPunch, 0.45f, true, 0.9f);
		}

		_special_state = SpecialState::TonguePull;
		_special_timer = FrogTuning::TONGUE_PULL_TIME;
	}

	void Frog::startSideHop() {
		const auto target = getTarget();
		if (!target)
			return;

		const Vector2 to_target = Vector2Normalize(Vector2Subtract(target->getCenter(), getCenter()));
		const float side = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;
		Vector2 lateral = {-to_target.y * side, to_target.x * side};
		if (Vector2LengthSqr(lateral) <= 0.001f)
			lateral = {side, 0.0f};

		Vector2 desired = {
			getCenter().x + lateral.x * FrogTuning::SIDEHOP_DISTANCE,
			getCenter().y + lateral.y * FrogTuning::SIDEHOP_DISTANCE
		};

		if (_map && _map->getNavMesh().isReady()) {
			const Vector3 snapped = _map->getNavMesh().getClosestWalkablePosition({desired.x, getAltitude(), desired.y});
			desired = {snapped.x, snapped.z};
		}

		_sidehop_target = desired;
		_special_state = SpecialState::SideHop;
		_special_timer = FrogTuning::SIDEHOP_DURATION;
		_sidehop_cooldown_timer = randomRange(1.8f, 3.0f);
		_tongue_cooldown_timer = std::max(_tongue_cooldown_timer, 0.7f);
		_state = State::Chasing;
		clearNavigationPath();
		setAnimationSpeed(1.25f);
		playAnimation("walk");
	}

	void Frog::startToxicPool() {
		const auto target = getTarget();
		if (!target)
			return;

		_toxic_pool_target_snapshot = target->getCenter();
		float toxic_pool_altitude = getAltitude();
		if (_map && _map->getNavMesh().isReady()) {
			const Vector3 snapped = _map->getNavMesh().getClosestWalkablePosition({
				_toxic_pool_target_snapshot.x,
				toxic_pool_altitude,
				_toxic_pool_target_snapshot.y
			});
			_toxic_pool_target_snapshot = {snapped.x, snapped.z};
			toxic_pool_altitude = snapped.y;
		}
		_special_state = SpecialState::ToxicSpitWindup;
		_special_timer = FrogTuning::TOXIC_POOL_WINDUP_TIME;
		_toxic_pool_cooldown_timer = randomRange(5.2f, 7.4f);
		_tongue_cooldown_timer = std::max(_tongue_cooldown_timer, 0.9f);
		_sidehop_cooldown_timer = std::max(_sidehop_cooldown_timer, 0.8f);
		_state = State::Chasing;
		stopMoving();
		clearNavigationPath();
		rotateTowardsCenter(_toxic_pool_target_snapshot.x, _toxic_pool_target_snapshot.y);
		beginCastTelemetry("Toxic Pool", FrogTuning::TOXIC_POOL_WINDUP_TIME, true);
		setAnimationSpeed(0.68f);
		playAnimation("attack", false, true, 0, true);
		playSoundEffect(Audio::SoundId::FrogSound, 0.78f, true, 0.82f);

		BossTelegraphHazardConfig config;
		config.name = getName() + " Toxic Pool";
		config.position = _toxic_pool_target_snapshot;
		config.altitude = toxic_pool_altitude;
		config.radius = FrogTuning::TOXIC_POOL_RADIUS;
		config.warning_seconds = FrogTuning::TOXIC_POOL_WINDUP_TIME;
		config.active_seconds = FrogTuning::TOXIC_POOL_DURATION;
		config.damage_per_tick = static_cast<int>(FrogTuning::TOXIC_POOL_DAMAGE * getDamageMultiplier());
		config.tick_interval = 0.85f;
		config.root_seconds_on_hit = 0.18f;
		config.source_context = Entity::makeDamageSourceContext(this, config.name);
		config.warning_color = {120, 220, 60, 255};
		config.active_color = {65, 180, 50, 255};
		addPendingSpawn(std::make_shared<BossTelegraphHazard>(std::move(config)));
	}

	void Frog::finishSpecialMove() {
		_special_state = SpecialState::None;
		_special_timer = 0.0f;
		_tongue_victim.reset();
		clearCastTelemetry();
		clearNavigationPath();
		_attack_cooldown_timer = std::max(_attack_cooldown_timer, 0.45f);
		_state = hasValidTarget() ? State::Chasing : State::Idle;
		setSpeedMultiplier(1.0f);
		setAnimationSpeed(1.0f);
		playAnimation(_state == State::Idle ? "idle" : "walk");
	}

	bool Frog::isTargetInTongueLane(const Entity& target) const {
		const Vector2 origin = getCenter();
		const Vector2 aim = getTongueAimDirection();
		const Vector2 to_target = Vector2Subtract(target.getCenter(), origin);
		const float forward = Vector2DotProduct(to_target, aim);
		if (forward < 1.25f || forward > FrogTuning::TONGUE_MAX_RANGE)
			return false;

		const Vector2 closest = {
			origin.x + aim.x * forward,
			origin.y + aim.y * forward
		};
		const float lateral_distance = Vector2Distance(target.getCenter(), closest);
		return lateral_distance <= FrogTuning::TONGUE_HALF_WIDTH + forward * 0.035f;
	}

	Vector2 Frog::getTongueAimDirection() const {
		const Vector2 origin = getCenter();
		Vector2 aim = Vector2Subtract(_tongue_target_snapshot, origin);
		if (Vector2LengthSqr(aim) <= 0.001f)
			return {1.0f, 0.0f};

		return Vector2Normalize(aim);
	}

} // namespace Nawia::Entity
