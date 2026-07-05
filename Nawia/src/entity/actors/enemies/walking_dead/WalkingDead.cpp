#include "WalkingDead.h"

#include <Collider.h>
#include <MathUtils.h>
#include <SoundIds.h>

#include <raymath.h>

namespace Nawia::Entity {

	namespace {
		constexpr const char* WALKING_DEAD_MODEL = "assets/models/actors/walking_dead/walking_dead.glb";
		constexpr int ANIM_DEATH = 1;
		constexpr int ANIM_HIT = 2;
		constexpr int ANIM_IDLE = 3;
		constexpr int ANIM_BITE = 4;
		constexpr int ANIM_SCREAM = 8;
		constexpr int ANIM_RUN_UP = 11;
		constexpr int ANIM_WALK = 13;
	}

	WalkingDead::WalkingDead() {
		setScale(1.5f);
		setFaction(Faction::Enemy);

		loadModel(WALKING_DEAD_MODEL);
		addAnimation("death", WALKING_DEAD_MODEL, ANIM_DEATH);
		addAnimation("get_hit", WALKING_DEAD_MODEL, ANIM_HIT);
		addAnimation("idle", WALKING_DEAD_MODEL, ANIM_IDLE);
		addAnimation("walk", WALKING_DEAD_MODEL, ANIM_WALK);
		addAnimation("run", WALKING_DEAD_MODEL, ANIM_RUN_UP);
		addAnimation("attack", WALKING_DEAD_MODEL, ANIM_BITE);
		addAnimation("scream", WALKING_DEAD_MODEL, ANIM_SCREAM);
		playAnimation("idle", true, false, 0, true);
	}

	void WalkingDead::takeDamage(const int dmg)
	{
		Entity::takeDamage(dmg);
		if (isDying()) return;

		const bool should_interrupt = _state == State::Attacking
			? GetRandomValue(1, 100) <= HIT_INTERRUPT_CHANCE
			: true;
		if (!should_interrupt)
			return;

		// Trafienie przerywa bieżącą akcję i odpala krótkie zachwianie.
		if (_state != State::GettingHit)
			_state_before_hit = _state;
		
		_state = State::GettingHit;
		setAnimationSpeed(HIT_REACTION_ANIMATION_SPEED);
		playAnimation("get_hit", false, true, 10, true);
		
		// Podczas zachwiania zatrzymujemy ruch.
		setVelocity(0, 0);
	}

	void WalkingDead::update(const float dt)
	{
		if (isDying())
		{
			Entity::update(dt);
			return;
		}

		if (isDormant()) return;
		updateAmbientSound(dt);

		// Aktualizacja czasu odnowienia ataku.
		if (_attack_cooldown_timer > 0.0f)
			_attack_cooldown_timer -= dt;

		switch (_state)
		{
		case State::Idle:
			handleIdleState(dt);
			break;
		case State::Chasing:
			handleChasingState(dt);
			break;
		case State::Attacking:
			handleAttackingState(dt);
			break;
		case State::Screaming:
			handleScreamingState(dt);
			break;
		case State::GettingHit:
			handleGettingHitState(dt);
			break;
		}
	}

	void WalkingDead::handleIdleState(const float dt)
	{
		Entity::update(dt);

		// Próba wykrycia celu.
		if (auto target = _target.lock())
		{
			const float dist = getDistanceToTarget();
			if (dist <= VISION_RANGE)
			{
				_state = State::Chasing;
				playAnimation("walk");
			}
		}
	}

	void WalkingDead::handleChasingState(const float dt)
	{
		Entity::update(dt);  // Bazowa aktualizacja animacji.

		auto target = _target.lock();
		if (!target || target->isDead())
		{
			_state = State::Screaming;
			playSoundEffect(Audio::SoundId::ZombieScream, 0.85f);
			playAnimation("scream", false, true);
			setVelocity(0, 0);
			stopMovement();
			return;
		}

		const float dist = getDistanceToTarget();

		// Sprawdzenie, czy cel uciekł poza zasięg widzenia.
		if (dist > VISION_RANGE * 1.5f)
		{
			_state = State::Screaming;
			playSoundEffect(Audio::SoundId::ZombieScream, 0.85f);
			playAnimation("scream", false, true);
			setVelocity(0, 0);
			stopMovement();
			return;
		}

		// Sprawdzenie, czy cel jest w zasięgu ataku.
		if (dist <= ATTACK_RANGE && _attack_cooldown_timer <= 0.0f)
		{
			_state = State::Attacking;
			_attack_damage_applied = false;
			setAnimationSpeed(ATTACK_ANIMATION_SPEED);
			playAnimation("attack", false, true);
			setVelocity(0, 0);
			stopMovement();
			return;
		}

		// Prędkość zależy od dystansu do celu.
		const bool should_run = dist <= CLOSE_RANGE;
		if (should_run != _is_running)
		{
			_is_running = should_run;
			setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
			playAnimation(_is_running ? "run" : "walk");
		}
		
		const float current_speed = _is_running ? RUN_SPEED : SPEED;
		setMovementSpeed(current_speed);
		
		// Pozycja celu używana przez ruch.
		const Vector2 target_pos = target->getCenter();
		
		// Z bliska używamy prostego ruchu bez przeliczania ścieżki.
		if (dist <= DIRECT_MOVE_DISTANCE)
		{
			stopMovement();  // Zatrzymujemy ruch po wyznaczonej ścieżce.
			const Vector2 my_pos = getCenter();
			const Vector2 dir = Vector2Normalize(Vector2Subtract(target_pos, my_pos));
			
			rotateTowards(target->getX(), target->getY());
			
			_pos.x += dir.x * current_speed * dt;
			_pos.y += dir.y * current_speed * dt;
		}
		else
		{
			// Dalej od celu wracamy do wyznaczania ścieżki.
			tickPathRecalcTimer(dt);
			
			if (isPathRecalcDue() || !isMoving())
			{
				moveTo(target_pos.x, target_pos.y);
				resetPathRecalcTimer(DEFAULT_PATH_RECALC_INTERVAL);
			}
			
			updateMovement(dt);
		}
	}

	void WalkingDead::handleAttackingState(const float dt)
	{
		Entity::update(dt);

		if (const auto target = _target.lock())
			rotateTowardsCenter(target->getCenter().x, target->getCenter().y);

		const int attack_frame_count = getAnimationFrameCount("attack");
		const float damage_frame = static_cast<float>(attack_frame_count) * ATTACK_DAMAGE_FRAME_RATIO;
		if (!_attack_damage_applied && attack_frame_count > 0 && _anim_frame_counter >= damage_frame)
		{
			if (const auto target = _target.lock())
			{
				if (getDistanceToTarget() <= ATTACK_RANGE * 1.7f)
				{
					target->rememberDamageSource(this, "Walking Dead Attack");
					target->takeDamage(ATTACK_DAMAGE);
					_attack_damage_applied = true;
				}
			}
		}

		if (!isAnimationLocked())
		{
			// Po zakończeniu animacji ataku zadajemy obrażenia.
			if (const auto target = _target.lock())
			{
				if (!_attack_damage_applied && getDistanceToTarget() <= ATTACK_RANGE * 1.7f)
				{
					target->rememberDamageSource(this, "Walking Dead Attack");
					target->takeDamage(ATTACK_DAMAGE);
				}
			}
			
			_attack_cooldown_timer = ATTACK_COOLDOWN;
			_state = State::Chasing;
			setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
			playAnimation("walk");
		}
	}

	void WalkingDead::handleScreamingState(const float dt)
	{
		Entity::update(dt);
		
		if (!isAnimationLocked())
		{
			// Po zakończeniu krzyku wracamy do bezczynności.
			_state = State::Idle;
			playAnimation("idle");
			_is_running = false;
		}
	}

	void WalkingDead::handleGettingHitState(const float dt)
	{
		Entity::update(dt);
		
		if (!isAnimationLocked())
		{
			// Po animacji trafienia wracamy do poprzedniego stanu.
			_state = _state_before_hit;
			
			// Przywracamy animację pasującą do stanu.
			switch (_state)
			{
			case State::Idle:
				setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
				playAnimation("idle");
				break;
			case State::Chasing:
				setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
				playAnimation(_is_running ? "run" : "walk");
				break;
			case State::Attacking:
				// Przerwany atak wraca do pościgu.
				_state = State::Chasing;
				_attack_cooldown_timer = ATTACK_COOLDOWN * 0.5f;
				_attack_damage_applied = false;
				setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
				playAnimation(_is_running ? "run" : "walk");
				break;
			case State::Screaming:
				setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
				playAnimation("scream", false, true);
				break;
			default:
				setAnimationSpeed(DEFAULT_ANIMATION_SPEED);
				playAnimation("idle");
				break;
			}
		}
	}

	void WalkingDead::updateAmbientSound(const float dt)
	{
		_ambient_sound_timer -= dt;
		if (_ambient_sound_timer > 0.0f)
			return;

		_ambient_sound_timer = static_cast<float>(GetRandomValue(700, 1700)) / 100.0f;
		if (GetRandomValue(0, 100) <= 35)
			playSoundEffect(Audio::SoundId::ZombieAmbient, 0.55f, false, static_cast<float>(GetRandomValue(90, 110)) / 100.0f);
	}

	void WalkingDead::onDeathStarted()
	{
		playSoundEffect(Audio::SoundId::ZombieDeath, 0.9f);
	}

} // namespace Nawia::Entity
