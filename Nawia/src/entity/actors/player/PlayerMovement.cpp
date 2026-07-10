#include "Player.h"

#include <Ability.h>
#include <SoundIds.h>

#include <algorithm>

namespace Nawia::Entity {

	void Player::moveTo(const float x, const float y) {
		if (isControlLocked()) {
			stop();
			return;
		}

		if (isMovementRooted()) {
			setMovementTarget(x, y);
			stopMovement();
			updateMovementSound(Audio::SoundPath::Footsteps, false);
			if (!isAnimationLocked()) {
				setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
				playAnimation("Idle_Loop");
			}
			return;
		}

		Entity::moveTo(x, y);

		if (isMoving()) {
			if (!isAnimationLocked()) {
				setAnimationSpeed(_current_stats.movement_speed * WALK_ANIM_BASE_SPEED);
				playAnimation("Walk_Loop");
			}
		} else {
			if (!isAnimationLocked()) {
				setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
				playAnimation("Idle_Loop");
			}
		}
	}

	void Player::applyRoot(const float duration) {
		Entity::applyRoot(duration);
		_path.clear();
		updateMovementSound(Audio::SoundPath::Footsteps, false);
		if (!isAnimationLocked()) {
			setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
			playAnimation("Idle_Loop");
		}
	}

	void Player::applyControlLock(const float duration) {
		if (duration <= 0.0f)
			return;

		_control_lock_timer = std::max(_control_lock_timer, duration);
		for (const auto& ability : getAbilities()) {
			if (ability)
				ability->cancel();
		}
		_is_consuming_food = false;
		_consume_food_timer = 0.0f;
		setVelocity(0.0f, 0.0f);
		stop();
		if (!isDying() && !_is_knocked_down) {
			setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
			playAnimation("Idle_Loop", true, false, 0, true);
		}
	}

	void Player::stop() {
		stopMovement();
		_path.clear();
		updateMovementSound(Audio::SoundPath::Footsteps, false);
		if (!isAnimationLocked()) {
			setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
			playAnimation("Idle_Loop");
		}
	}

	void Player::clearControlLocks() {
		clearStatusEffects();
		_is_knocked_down = false;
		_knockdown_phase = KnockdownPhase::None;
		_is_consuming_food = false;
		_consume_food_timer = 0.0f;
		_control_lock_timer = 0.0f;
		stop();
		setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
		if (!isDying())
			playAnimation("Idle_Loop", true, false, 0, true);
	}

	void Player::update(const float delta_time) {
		Entity::update(delta_time);

		if (isDying()) return;

		updateAbilities(delta_time);

		if (_control_lock_timer > 0.0f) {
			_control_lock_timer = std::max(0.0f, _control_lock_timer - delta_time);
			updateMovementSound(Audio::SoundPath::Footsteps, false);
			return;
		}

		if (_is_consuming_food) {
			_consume_food_timer -= delta_time;
			if (_consume_food_timer <= 0.0f) {
				_is_consuming_food = false;
				_consume_food_timer = 0.0f;
				(void)consumeFood();
				setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
				if (!isAnimationLocked())
					playAnimation("Idle_Loop");
			}
			return;
		}

		isLevelUp();
		if (_is_knocked_down) {
			if (!isAnimationLocked()) {
				if (_knockdown_phase == KnockdownPhase::Knocked) {
					_knockdown_phase = KnockdownPhase::StandingUp;
					playAnimation("LayToIdle", false, true, 0, true);
				} else {
					_is_knocked_down = false;
					_knockdown_phase = KnockdownPhase::None;
					setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
					playAnimation("Idle_Loop");
				}
			}
			return;
		}

		updateMovement(delta_time);
	}

	void Player::updateMovement(const float delta_time) {
		if (!isMoving() || _is_knocked_down || isMovementRooted()) {
			updateMovementSound(Audio::SoundPath::Footsteps, false);
			return;
		}

		if (!isAnimationLocked()) {
			setAnimationSpeed(_current_stats.movement_speed * WALK_ANIM_BASE_SPEED);
			playAnimation("Walk_Loop");
		}

		Entity::updateMovement(delta_time);
		updateMovementSound(Audio::SoundPath::Footsteps, isMoving() && !isAnimationLocked(), 0.48f);

		if (!isMoving() && !isAnimationLocked())
			playAnimation("Idle_Loop");
	}

	void Player::knockDown(const int damage) {
		if (_is_knocked_down) {
			takeDamage(damage);
			return;
		}

		stop();
		takeDamage(damage);

		_is_knocked_down = true;
		_knockdown_phase = KnockdownPhase::Knocked;
		setAnimationSpeed(4.0f);
		playAnimation("Hit_Knockback", false, true, 0, true);
	}

	void Player::respawn() {
		clearStatusEffects();
		setHP(std::max(1, getMaxHP() / 2));
		_is_knocked_down = false;
		_knockdown_phase = KnockdownPhase::None;
		_control_lock_timer = 0.0f;
		stopMovement();
		setX(_respawn_point.x);
		setY(_respawn_point.y);
		setMovementTarget(_respawn_point.x, _respawn_point.y);
		_path.clear();
		setFaction(Faction::Player);
		setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
		playAnimation("Idle_Loop");
	}

	void Player::onDeathStarted() {
		updateMovementSound(Audio::SoundPath::Footsteps, false);
		playSoundEffect(Audio::SoundId::HumanDeath, 0.9f);
	}

} // namespace Nawia::Entity
