#include "RiftBinder.h"
#include "RiftBinderInternal.h"

#include <SoundIds.h>

#include <algorithm>

namespace Nawia::Entity {

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

		faceTargetCenter();

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

		faceTargetCenter();

		if (!_melee_damage_applied && hasAnimationReachedRatio("attack", MELEE_DAMAGE_FRAME_RATIO))
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

	float RiftBinder::actionCooldownDuration() const
	{
		return _shield_active ? SHIELDED_ACTION_COOLDOWN : ACTION_COOLDOWN;
	}

} // namespace Nawia::Entity
