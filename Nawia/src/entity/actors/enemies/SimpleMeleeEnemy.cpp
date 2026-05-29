#include "SimpleMeleeEnemy.h"

#include <raymath.h>

namespace Nawia::Entity {

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
			setVelocity(0.0f, 0.0f);
			_is_moving = false;
			setAnimationSpeed(1.0f);
			playAnimation(_idle_animation);
			return;
		}

		const float distance = getDistanceToTarget();
		if (distance > _vision_range * 1.6f) {
			_state = State::Idle;
			setVelocity(0.0f, 0.0f);
			_is_moving = false;
			setAnimationSpeed(1.0f);
			playAnimation(_idle_animation);
			return;
		}

		if (distance <= _attack_range && _attack_cooldown_timer <= 0.0f) {
			_state = State::Attacking;
			_attack_damage_applied = false;
			setVelocity(0.0f, 0.0f);
			_is_moving = false;
			setAnimationSpeed(_attack_animation_speed);
			playAnimation(_attack_animation, false, true, 0, true);
			return;
		}

		const Vector2 target_pos = getTargetPosition();
		moveTo(target_pos.x, target_pos.y);
		updateMovement(dt);

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
		setAnimationSpeed(1.0f);
		playAnimation(_state == State::Idle ? _idle_animation : _walk_animation);
	}

} // namespace Nawia::Entity
