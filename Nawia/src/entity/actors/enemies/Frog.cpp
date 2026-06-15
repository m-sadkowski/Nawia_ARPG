#include "Frog.h"

#include <Map.h>
#include <SoundIds.h>
#include <VillageHeadNpc.h>

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <memory>

namespace Nawia::Entity {

	namespace {
		constexpr const char* MODEL_PATH = "assets/models/actors/frog/frog.glb";
		constexpr float TONGUE_MIN_RANGE = 4.5f;
		constexpr float TONGUE_MAX_RANGE = 13.0f;
		constexpr float TONGUE_HALF_WIDTH = 1.15f;
		constexpr float TONGUE_WINDUP_TIME = 0.38f;
		constexpr float TONGUE_PULL_TIME = 0.34f;
		constexpr float TONGUE_RECOVER_TIME = 0.38f;
		constexpr float TONGUE_PULL_SPEED = 24.0f;
		constexpr float TONGUE_STOP_DISTANCE = 2.75f;
		constexpr int TONGUE_DAMAGE = 8;
		constexpr float SIDEHOP_DURATION = 0.48f;
		constexpr float SIDEHOP_DISTANCE = 4.25f;
	}

	Frog::Frog() {
		setScale(1.5f);
		loadModel(MODEL_PATH);
		addAnimation("attack", MODEL_PATH, 0);
		addAnimation("death", MODEL_PATH, 1);
		addAnimation("idle", MODEL_PATH, 2);
		addAnimation("walk", MODEL_PATH, 3);
		configureAnimations("idle", "walk", "attack");
		configureCombat(18.0f, 3.8f, 4.2f, 20, 0.8f, 2.15f, 0.32f);
		playAnimation("idle", true, false, 0, true);
	}

	void Frog::update(const float dt) {
		if (isDying()) {
			Entity::update(dt);
			return;
		}

		if (isDormant())
			return;

		if (_special_state != SpecialState::None) {
			updateSpecialState(dt);
			return;
		}

		// Po trafieniu Frog robi krotki odskok, zeby walka nie byla statycznym klepaniem w miejscu.
		if (_retreat_timer > 0.0f && !isDying()) {
			_retreat_timer -= dt;

			setSpeedMultiplier(1.75f);
			if (!isAnimationLocked() && getAnimationFrameCount("walk") > 0)
				playAnimation("walk");

			Entity::update(dt);

			if (hasValidTarget()) {
				const auto target = getTarget();
				Vector2 away = {
					getCenter().x - target->getCenter().x,
					getCenter().y - target->getCenter().y
				};
				const float length = std::sqrt(away.x * away.x + away.y * away.y);
				if (length > 0.001f) {
					away.x /= length;
					away.y /= length;
				} else {
					away = {1.0f, 0.0f};
				}

				Vector3 retreat_position = {
					getX() + away.x * 5.0f,
					getAltitude(),
					getY() + away.y * 5.0f
				};
				if (_map && _map->getNavMesh().isReady())
					retreat_position = _map->getNavMesh().getClosestWalkablePosition(retreat_position);

				moveTowardPositionWithNav({retreat_position.x, retreat_position.z}, dt);
			} else {
				clearNavigationPath();
				_is_moving = false;
			}

			updateMovementSound(Audio::SoundPath::DevilStep, _is_moving && !isDormant(), 0.32f, 1.25f);
			if (_retreat_timer <= 0.0f || !_is_moving) {
				_retreat_timer = 0.0f;
				clearNavigationPath();
				_attack_cooldown_timer = std::max(_attack_cooldown_timer, 1.0f);
				_state = State::Chasing;
				setAnimationSpeed(1.0f);
			}
			return;
		}

		if (_tongue_cooldown_timer > 0.0f)
			_tongue_cooldown_timer -= dt;
		if (_sidehop_cooldown_timer > 0.0f)
			_sidehop_cooldown_timer -= dt;

		tryStartSpecialMove();
		if (_special_state != SpecialState::None) {
			updateSpecialState(dt);
			return;
		}

		if (hasValidTarget() && getDistanceToTarget() > 8.0f)
			setSpeedMultiplier(1.65f);
		else
			setSpeedMultiplier(1.0f);

		const State previous_state = _state;
		SimpleMeleeEnemy::update(dt);
		updateMovementSound(Audio::SoundPath::DevilStep, _is_moving && !isDying() && !isDormant(), 0.30f, 1.25f);

		if (previous_state == State::Idle && _state == State::Chasing)
			playSoundEffect(Audio::SoundId::DevilAggro, 0.72f, true, 0.82f);
	}

	void Frog::render(const Camera3D& camera) {
		Entity::render(camera);

		if (_special_state != SpecialState::TongueWindup &&
			_special_state != SpecialState::TonguePull &&
			_special_state != SpecialState::TongueRecover) {
			return;
		}

		const Vector2 origin_2d = getCenter();
		Vector2 end_2d = _tongue_target_snapshot;
		if (const auto victim = _tongue_victim.lock())
			end_2d = victim->getCenter();
		else if (_special_state == SpecialState::TongueWindup) {
			const Vector2 aim = getTongueAimDirection();
			end_2d = {
				origin_2d.x + aim.x * TONGUE_MAX_RANGE,
				origin_2d.y + aim.y * TONGUE_MAX_RANGE
			};
		}

		const float height = getAltitude() + 1.05f;
		const Vector3 origin = {origin_2d.x, height, origin_2d.y};
		const Vector3 end = {end_2d.x, height, end_2d.y};
		const Color color = _special_state == SpecialState::TongueWindup
			? Fade(RED, 0.48f)
			: Fade(MAROON, 0.88f);

		DrawCylinderEx(origin, end, 0.07f, 0.04f, 10, color);
		DrawSphere(end, 0.16f, color);
	}

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
			_special_timer = TONGUE_RECOVER_TIME;
			return;
		}

		const Vector2 frog_center = getCenter();
		const Vector2 victim_center = victim->getCenter();
		const Vector2 toward_frog = Vector2Normalize(Vector2Subtract(frog_center, victim_center));
		const float distance = Vector2Distance(frog_center, victim_center);
		if (distance > TONGUE_STOP_DISTANCE) {
			const float move_amount = std::min(TONGUE_PULL_SPEED * dt, distance - TONGUE_STOP_DISTANCE);
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
			_special_timer = TONGUE_RECOVER_TIME;
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
		if (_is_moving) {
			setAnimationSpeed(1.25f);
			playAnimation("walk");
		}

		if (const auto target = getTarget())
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);

		updateMovementSound(Audio::SoundPath::DevilStep, _is_moving && !isDormant(), 0.28f, 1.45f);
		if (_special_timer <= 0.0f || !_is_moving)
			finishSpecialMove();
	}

	void Frog::tryStartSpecialMove() {
		if (!hasValidTarget() || isAnimationLocked() || _state == State::GettingHit)
			return;

		const float distance = getDistanceToTarget();
		if (_tongue_cooldown_timer <= 0.0f &&
			distance >= TONGUE_MIN_RANGE &&
			distance <= TONGUE_MAX_RANGE &&
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
		_special_timer = TONGUE_WINDUP_TIME;
		_tongue_cooldown_timer = randomRange(3.6f, 5.4f);
		_sidehop_cooldown_timer = std::max(_sidehop_cooldown_timer, 0.8f);
		_state = State::Chasing;
		stopMoving();
		clearNavigationPath();
		rotateTowardsCenter(_tongue_target_snapshot.x, _tongue_target_snapshot.y);
		setAnimationSpeed(0.72f);
		playAnimation("attack", false, true, 0, true);
		playSoundEffect(Audio::SoundId::DevilAggro, 0.58f, true, 1.08f);
	}

	void Frog::releaseTongueStrike() {
		const auto target = getTarget();
		if (target && isTargetInTongueLane(*target)) {
			target->rememberDamageSource(this);
			target->takeDamage(static_cast<int>(TONGUE_DAMAGE * _damage_multiplier));
			target->applyRoot(TONGUE_PULL_TIME + 0.18f);
			_tongue_victim = target;
			playSoundEffect(Audio::SoundId::DevilDashHit, 0.72f, true, 1.25f);
		} else {
			_tongue_victim.reset();
			playSoundEffect(Audio::SoundId::DevilPunch, 0.45f, true, 0.9f);
		}

		_special_state = SpecialState::TonguePull;
		_special_timer = TONGUE_PULL_TIME;
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
			getCenter().x + lateral.x * SIDEHOP_DISTANCE,
			getCenter().y + lateral.y * SIDEHOP_DISTANCE
		};

		if (_map && _map->getNavMesh().isReady()) {
			const Vector3 snapped = _map->getNavMesh().getClosestWalkablePosition({desired.x, getAltitude(), desired.y});
			desired = {snapped.x, snapped.z};
		}

		_sidehop_target = desired;
		_special_state = SpecialState::SideHop;
		_special_timer = SIDEHOP_DURATION;
		_sidehop_cooldown_timer = randomRange(1.8f, 3.0f);
		_tongue_cooldown_timer = std::max(_tongue_cooldown_timer, 0.7f);
		_state = State::Chasing;
		clearNavigationPath();
		setAnimationSpeed(1.25f);
		playAnimation("walk");
	}

	void Frog::finishSpecialMove() {
		_special_state = SpecialState::None;
		_special_timer = 0.0f;
		_tongue_victim.reset();
		clearNavigationPath();
		_attack_cooldown_timer = std::max(_attack_cooldown_timer, 0.45f);
		_state = hasValidTarget() ? State::Chasing : State::Idle;
		setSpeedMultiplier(1.0f);
		setAnimationSpeed(1.0f);
		playAnimation(_state == State::Idle ? "idle" : "walk");
	}

	void Frog::stopMoving() {
		setVelocity(0.0f, 0.0f);
		_is_moving = false;
	}

	bool Frog::isTargetInTongueLane(const Entity& target) const {
		const Vector2 origin = getCenter();
		const Vector2 aim = getTongueAimDirection();
		const Vector2 to_target = Vector2Subtract(target.getCenter(), origin);
		const float forward = Vector2DotProduct(to_target, aim);
		if (forward < 1.25f || forward > TONGUE_MAX_RANGE)
			return false;

		const Vector2 closest = {
			origin.x + aim.x * forward,
			origin.y + aim.y * forward
		};
		const float lateral_distance = Vector2Distance(target.getCenter(), closest);
		return lateral_distance <= TONGUE_HALF_WIDTH + forward * 0.035f;
	}

	Vector2 Frog::getTongueAimDirection() const {
		const Vector2 origin = getCenter();
		Vector2 aim = Vector2Subtract(_tongue_target_snapshot, origin);
		if (Vector2LengthSqr(aim) <= 0.001f)
			return {1.0f, 0.0f};

		return Vector2Normalize(aim);
	}

	float Frog::randomRange(const float min, const float max) const {
		const int roll = GetRandomValue(0, 1000);
		const float t = static_cast<float>(roll) / 1000.0f;
		return min + (max - min) * t;
	}

	void Frog::onAttackDamageApplied(Entity& target) {
		playSoundEffect(Audio::SoundId::DevilPunch, 0.82f, true, 1.15f);
		_retreat_timer = 1.25f;
		clearNavigationPath();
		_attack_cooldown_timer = std::max(_attack_cooldown_timer, 1.15f);
		rotateTowardsCenter(target.getCenter().x, target.getCenter().y);
	}

	void Frog::onDeathStarted() {
		playSoundEffect(Audio::SoundId::DevilDeath, 0.85f, true, 0.9f);

		if (!_engine)
			return;

		// Wczesny boss Frog jest przejsciem do sceny z Soltysem, wiec NPC pojawia sie w miejscu smierci.
		auto village_head = std::make_shared<VillageHeadNpc>("Soltys", getX(), getY(), _engine);
		village_head->setAltitude(getAltitude());
		village_head->setAudioManager(_audio_manager);
		addPendingSpawn(village_head);
	}

} // namespace Nawia::Entity
