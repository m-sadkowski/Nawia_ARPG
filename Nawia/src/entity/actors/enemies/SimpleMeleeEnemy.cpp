#include "SimpleMeleeEnemy.h"

#include <Map.h>

#include <raymath.h>

namespace Nawia::Entity {

	namespace {
		constexpr float NAV_PATH_POINT_REACHED_DISTANCE_SQ = 0.16f;
		constexpr float NAV_PATH_TARGET_CHANGE_DISTANCE_SQ = 0.64f;
	}

	SimpleMeleeEnemy::SimpleMeleeEnemy() {
		setFaction(Faction::Enemy);
	}

	void SimpleMeleeEnemy::configureCombat(
		const float vision_range,
		const float attack_range,
		const float movement_speed,
		const int attack_damage,
		const float attack_cooldown,
		const float attack_animation_speed,
		const float attack_damage_frame_ratio
	) {
		_vision_range = vision_range;
		_attack_range = attack_range;
		_base_movement_speed = movement_speed;
		_attack_damage = attack_damage;
		_attack_cooldown = attack_cooldown;
		_attack_animation_speed = attack_animation_speed;
		_attack_damage_frame_ratio = attack_damage_frame_ratio;
		setMovementSpeed(_base_movement_speed);
	}

	void SimpleMeleeEnemy::configureAnimations(
		const std::string& idle_animation,
		const std::string& walk_animation,
		const std::string& attack_animation,
		const std::string& hit_animation
	) {
		_idle_animation = idle_animation;
		_walk_animation = walk_animation;
		_attack_animation = attack_animation;
		_hit_animation = hit_animation;
	}

	void SimpleMeleeEnemy::takeDamage(const int dmg) {
		Entity::takeDamage(dmg);
		if (isDying() || _hit_animation.empty() || getAnimationFrameCount(_hit_animation) <= 0)
			return;

		if (_state != State::GettingHit)
			_state_before_hit = _state;

		_state = State::GettingHit;
		clearNavigationPath();
		setVelocity(0.0f, 0.0f);
		_is_moving = false;
		setAnimationSpeed(1.0f);
		playAnimation(_hit_animation, false, true, 0, true);
	}

	void SimpleMeleeEnemy::update(const float dt) {
		if (isDying()) {
			Entity::update(dt);
			return;
		}

		if (isDormant())
			return;

		if (_attack_cooldown_timer > 0.0f)
			_attack_cooldown_timer -= dt;

		switch (_state) {
			case State::Idle:
				handleIdleState(dt);
				break;
			case State::Chasing:
				handleChasingState(dt);
				break;
			case State::Attacking:
				handleAttackingState(dt);
				break;
			case State::GettingHit:
				handleGettingHitState(dt);
				break;
		}
	}

	void SimpleMeleeEnemy::handleIdleState(const float dt) {
		Entity::update(dt);

		if (hasValidTarget() && getDistanceToTarget() <= _vision_range) {
			_state = State::Chasing;
			setAnimationSpeed(1.0f);
			playAnimation(_walk_animation);
		}
	}

	void SimpleMeleeEnemy::handleChasingState(const float dt) {
		Entity::update(dt);

		if (!hasValidTarget()) {
			_state = State::Idle;
			clearNavigationPath();
			setVelocity(0.0f, 0.0f);
			_is_moving = false;
			setAnimationSpeed(1.0f);
			playAnimation(_idle_animation);
			return;
		}

		const float distance = getDistanceToTarget();
		if (distance > _vision_range * 1.6f) {
			_state = State::Idle;
			clearNavigationPath();
			setVelocity(0.0f, 0.0f);
			_is_moving = false;
			setAnimationSpeed(1.0f);
			playAnimation(_idle_animation);
			return;
		}

		const Vector2 target_pos = getTargetPosition();
		if (distance <= _attack_range && _attack_cooldown_timer <= 0.0f && canReachPositionWithNav(target_pos)) {
			_state = State::Attacking;
			_attack_damage_applied = false;
			clearNavigationPath();
			setVelocity(0.0f, 0.0f);
			_is_moving = false;
			setAnimationSpeed(_attack_animation_speed);
			playAnimation(_attack_animation, false, true, 0, true);
			return;
		}

		moveTowardPositionWithNav(target_pos, dt);

		if (_is_moving) {
			setAnimationSpeed(1.0f);
			playAnimation(_walk_animation);
		}
	}

	void SimpleMeleeEnemy::handleAttackingState(const float dt) {
		Entity::update(dt);

		if (const auto target = _target.lock())
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);

		const int attack_frame_count = getAnimationFrameCount(_attack_animation);
		const float damage_frame = static_cast<float>(attack_frame_count) * _attack_damage_frame_ratio;
		if (!_attack_damage_applied && attack_frame_count > 0 && _anim_frame_counter >= damage_frame) {
			if (const auto target = _target.lock()) {
				if (getDistanceToTarget() <= _attack_range * 1.6f) {
					target->takeDamage(static_cast<int>(_attack_damage * _damage_multiplier));
					_attack_damage_applied = true;
					onAttackDamageApplied(*target);
				}
			}
		}

		if (!isAnimationLocked()) {
			_attack_cooldown_timer = _attack_cooldown;
			_state = State::Chasing;
			setAnimationSpeed(1.0f);
			playAnimation(_walk_animation);
		}
	}

	void SimpleMeleeEnemy::handleGettingHitState(const float dt) {
		Entity::update(dt);

		if (isAnimationLocked())
			return;

		_state = _state_before_hit == State::Attacking ? State::Chasing : _state_before_hit;
		clearNavigationPath();
		setAnimationSpeed(1.0f);
		playAnimation(_state == State::Idle ? _idle_animation : _walk_animation);
	}

	bool SimpleMeleeEnemy::moveTowardPositionWithNav(const Vector2 target_pos, const float dt, const float repath_interval) {
		if (!_map || !_map->getNavMesh().isReady()) {
			clearNavigationPath();
			moveTo(target_pos.x, target_pos.y);
			updateMovement(dt);
			return _is_moving;
		}

		_path_recalc_timer -= dt;
		const bool target_changed = !_has_current_nav_target ||
			Vector2DistanceSqr(_current_nav_target, target_pos) > NAV_PATH_TARGET_CHANGE_DISTANCE_SQ;

		if (_path_recalc_timer <= 0.0f || target_changed) {
			_current_nav_path = _map->findPath(getWorldPos3D(), {target_pos.x, getAltitude(), target_pos.y});
			_current_nav_target = target_pos;
			_has_current_nav_target = true;
			_path_recalc_timer = repath_interval;
		}

		const Vector2 current_pos = getCenter();
		while (!_current_nav_path.empty() &&
			   Vector2DistanceSqr(current_pos, _current_nav_path.front()) <= NAV_PATH_POINT_REACHED_DISTANCE_SQ) {
			_current_nav_path.erase(_current_nav_path.begin());
		}

		if (_current_nav_path.empty()) {
			setVelocity(0.0f, 0.0f);
			_is_moving = false;
			return false;
		}

		moveTo(_current_nav_path.front().x, _current_nav_path.front().y);
		updateMovement(dt);
		return _is_moving;
	}

	bool SimpleMeleeEnemy::canReachPositionWithNav(const Vector2 target_pos) const {
		if (!_map || !_map->getNavMesh().isReady())
			return true;

		return !_map->findPath(getWorldPos3D(), {target_pos.x, getAltitude(), target_pos.y}).empty();
	}

	void SimpleMeleeEnemy::clearNavigationPath() {
		_current_nav_path.clear();
		_has_current_nav_target = false;
		_path_recalc_timer = 0.0f;
	}

} // namespace Nawia::Entity
