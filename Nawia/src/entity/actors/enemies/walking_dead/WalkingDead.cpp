#include "WalkingDead.h"

#include <Collider.h>
#include <MathUtils.h>
#include <SoundIds.h>

#include <raymath.h>

namespace Nawia::Entity {

	WalkingDead::WalkingDead() {
		setScale(0.015f);
		setFaction(Faction::Enemy);

		loadModel("assets/models/walking_dead_idle.glb");
		addAnimation("idle", "assets/models/walking_dead_idle.glb");
		addAnimation("walk", "assets/models/walking_dead_walk.glb");
		addAnimation("run", "assets/models/walking_dead_run.glb");
		addAnimation("attack", "assets/models/walking_dead_attack.glb");
		addAnimation("death", "assets/models/walking_dead_death.glb");
		addAnimation("scream", "assets/models/walking_dead_scream.glb");
		addAnimation("get_hit", "assets/models/walking_dead_hit.glb");
	}

	void WalkingDead::takeDamage(const int dmg)
	{
		Entity::takeDamage(dmg);
		if (isDying()) return;

		// Trafienie przerywa bieżącą akcję i odpala krótkie zachwianie.
		if (_state != State::GettingHit)
			_state_before_hit = _state;
		
		_state = State::GettingHit;
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
			_is_moving = false;
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
			_is_moving = false;
			return;
		}

		// Sprawdzenie, czy cel jest w zasięgu ataku.
		if (dist <= ATTACK_RANGE && _attack_cooldown_timer <= 0.0f)
		{
			_state = State::Attacking;
			playAnimation("attack", false, true);
			setVelocity(0, 0);
			_is_moving = false;
			return;
		}

		// Prędkość zależy od dystansu do celu.
		const bool should_run = dist <= CLOSE_RANGE;
		if (should_run != _is_running)
		{
			_is_running = should_run;
			playAnimation(_is_running ? "run" : "walk");
		}
		
		const float current_speed = _is_running ? RUN_SPEED : SPEED;
		setMovementSpeed(current_speed);
		
		// Pozycja celu używana przez ruch.
		const Vector2 target_pos = target->getCenter();
		
		// Z bliska używamy prostego ruchu bez przeliczania ścieżki.
		if (dist <= DIRECT_MOVE_DISTANCE)
		{
			_is_moving = false;  // Zatrzymujemy ruch po wyznaczonej ścieżce.
			const Vector2 my_pos = getCenter();
			const Vector2 dir = Vector2Normalize(Vector2Subtract(target_pos, my_pos));
			
			rotateTowards(target->getX(), target->getY());
			
			_pos.x += dir.x * current_speed * dt;
			_pos.y += dir.y * current_speed * dt;
		}
		else
		{
			// Dalej od celu wracamy do wyznaczania ścieżki.
			_path_recalc_timer -= dt;
			
			if (_path_recalc_timer <= 0.0f || !_is_moving)
			{
				moveTo(target_pos.x, target_pos.y);
				_path_recalc_timer = DEFAULT_PATH_RECALC_INTERVAL;
			}
			
			updateMovement(dt);
		}
	}

	void WalkingDead::handleAttackingState(const float dt)
	{
		Entity::update(dt);

		if (!isAnimationLocked())
		{
			// Po zakończeniu animacji ataku zadajemy obrażenia.
			if (const auto target = _target.lock())
			{
				if (getDistanceToTarget() <= ATTACK_RANGE * 1.5f)
				{
					target->takeDamage(ATTACK_DAMAGE);
				}
			}
			
			_attack_cooldown_timer = ATTACK_COOLDOWN;
			_state = State::Chasing;
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
				playAnimation("idle");
				break;
			case State::Chasing:
				playAnimation(_is_running ? "run" : "walk");
				break;
			case State::Attacking:
				// Przerwany atak wraca do pościgu.
				_state = State::Chasing;
				playAnimation(_is_running ? "run" : "walk");
				break;
			case State::Screaming:
				playAnimation("scream", false, true);
				break;
			default:
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
