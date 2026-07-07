#include "Bandit.h"

#include <Ability.h>
#include <Collider.h>
#include <KnifeThrowAbility.h>
#include <Map.h>
#include <MathUtils.h>
#include <SoundIds.h>

#include <raymath.h>

#include <cmath>

namespace Nawia::Entity {

	Bandit::Bandit() {
		setScale(0.015f);
		setFaction(Faction::Enemy);

		loadModel("assets/models/actors/bandit/bandit_idle.glb");
		addAnimation("idle", "assets/models/actors/bandit/bandit_idle.glb");
		addAnimation("walk", "assets/models/actors/bandit/bandit_walk_backwards3.glb");
		addAnimation("throw", "assets/models/actors/bandit/bandit_throw.glb");
		addAnimation("death", "assets/models/actors/bandit/bandit_death.glb");
	}

	void Bandit::ensureKnifeThrowAbility(Core::ResourceManager* resource_manager) {
		setAbility(0, std::make_shared<KnifeThrowAbility>(
			"assets/models/knife.glb",
			0.05f,
			nullptr,
			nullptr,
			180.0f,
			resource_manager));
	}

	void Bandit::takeDamage(const int dmg)
	{
		if (_state == State::Casting && !_knife_thrown_this_cast)
			_pending_interrupted_knife_throw = true;

		Entity::takeDamage(dmg);
		
		if (isDying()) {
			_pending_interrupted_knife_throw = false;
			return;
		}
		
		if (_state != State::GettingHit)
			_state_before_hit = _state;
		
		_state = State::GettingHit;
		stopMotion();
	}

	void Bandit::update(const float dt)
	{
		if (isDying())
		{
			updateMovementSound(Audio::SoundPath::Footsteps, false);
			Entity::update(dt);
			return;
		}

		if (isDormant()) return;

		if (_knife_cooldown_timer > 0.0f)
			_knife_cooldown_timer -= dt;

		switch (_state)
		{
		case State::Idle:
			handleIdleState(dt);
			break;
		case State::Chasing:
			handleChasingState(dt);
			break;
		case State::Casting:
			handleCastingState(dt);
			break;
		case State::GettingHit:
			handleGettingHitState(dt);
			break;
		}
	}

	void Bandit::handleIdleState(const float dt)
	{
		Entity::update(dt);

		if (auto target = getTarget())
		{
			const float dist = getDistanceToTarget();
			if (dist <= VISION_RANGE)
			{
				_state = State::Chasing;
				playAnimation("idle");
			}
		}
	}

	void Bandit::handleChasingState(const float dt) {
		Entity::update(dt);  // Bazowa aktualizacja animacji.
		updateAbilities(dt);

		auto target = getTarget();
		if (!target || target->isDead()) {
			_state = State::Idle;
			playAnimation("idle");
			stopMotion();
			updateMovementSound(Audio::SoundPath::Footsteps, false);
			return;
		}

		const float dist = getDistanceToTarget();

		if (dist > VISION_RANGE * 1.5f) {
			_state = State::Idle;
			playAnimation("idle");
			stopMotion();
			updateMovementSound(Audio::SoundPath::Footsteps, false);
			return;
		}

		// Sprawdzenie gotowości rzutu nożem.
		if (dist <= ATTACK_RANGE && _knife_cooldown_timer <= 0.0f) {
			if (const auto knife = getAbility(0)) {
				_state = State::Casting;
				_knife_thrown_this_cast = false;  // Reset flagi dla nowej sekwencji rzutu.
				_pending_interrupted_knife_throw = false;
				setAnimationSpeed(THROW_ANIMATION_SPEED);
				playAnimation("throw", false, true);
				rotateTowards(target->getX(), target->getY());
				stopMotion();
				updateMovementSound(Audio::SoundPath::Footsteps, false);
				return;
			}
		}

		const Vector2 my_pos = getCenter();
		const Vector2 target_pos = target->getCenter();
		
		setMovementSpeed(SPEED);
		
		// Cel jest za blisko, więc bandyta wycofuje się z użyciem wyznaczania ścieżki.
		if (dist < MIN_DISTANCE) 
		{
			tickPathRecalcTimer(dt);
			
			if (isPathRecalcDue() || !isMoving() || !_is_retreating)
			{
				// Punkt odwrotu leży kilka jednostek od celu w przeciwnym kierunku.
				const Vector2 away_dir = Vector2Normalize(Vector2Subtract(my_pos, target_pos));
				constexpr float retreat_dist = 3.0f;
				
				// Najpierw próbujemy znaleźć punkt odwrotu, po którym da się chodzić.
				Vector2 retreat_point = { my_pos.x + away_dir.x * retreat_dist, my_pos.y + away_dir.y * retreat_dist };
				
				// Jeśli punkt jest zablokowany, próbujemy alternatywnych kątów.
				if (_map && !_map->isWalkable(retreat_point.x, retreat_point.y))
				{
					// Odchylenia o około 45 i 90 stopni zwiększają szansę znalezienia obejścia.
					for (const float angle_offset : { 0.785f, -0.785f, 1.57f, -1.57f }) 
					{
						const float angle = std::atan2(away_dir.y, away_dir.x) + angle_offset;
						retreat_point = {
							my_pos.x + static_cast<float>(std::cos(angle)) * retreat_dist,
							my_pos.y + static_cast<float>(std::sin(angle)) * retreat_dist
						};
						if (_map->isWalkable(retreat_point.x, retreat_point.y)) break;
					}
				}
				
				moveTo(retreat_point.x, retreat_point.y);
				resetPathRecalcTimer(PATH_RECALC_INTERVAL);
				_is_retreating = true;
			}
			
			playAnimation("walk");
			updateMovement(dt);
			updateMovementSound(Audio::SoundPath::Footsteps, isMoving(), 0.42f, 1.08f);
			rotateTowards(target->getX(), target->getY());  // Podczas odwrotu nadal patrzymy na cel.
		}
		// Cel jest za daleko, więc bandyta podchodzi.
		else if (dist > ATTACK_RANGE) 
		{
			tickPathRecalcTimer(dt);
			
			if (isPathRecalcDue() || !isMoving() || _is_retreating)
			{
				moveTo(target_pos.x, target_pos.y);
				resetPathRecalcTimer(PATH_RECALC_INTERVAL);
				_is_retreating = false;
			}
			
			playAnimation("walk");
			updateMovement(dt);
			updateMovementSound(Audio::SoundPath::Footsteps, isMoving(), 0.42f, 1.08f);
		}
		// Dystans jest dobry, więc bandyta stoi i celuje.
		else 
		{
			playAnimation("idle");
			stopMovement();
			_is_retreating = false;
			updateMovementSound(Audio::SoundPath::Footsteps, false);
		}
		
		rotateTowards(target->getX(), target->getY());
	}

	void Bandit::handleCastingState(const float dt)
	{
		Entity::update(dt);
		updateAbilities(dt);

		// Podczas animacji rzutu bandyta dalej obraca się w stronę celu.
		if (auto target = getTarget())
		{
			rotateTowards(target->getX(), target->getY());
		}

		// Nóż rzucamy tylko raz, w konkretnej fazie animacji.
		if (!_knife_thrown_this_cast && hasAnimationReachedRatio("throw", THROW_SPAWN_FRAME_RATIO))
		{
			if (const auto knife = getAbility(0))
			{
				if (const auto target = getTarget())
				{
					const float tx = target->getCenter().x;
					const float ty = target->getCenter().y;

					if (auto effect = knife->cast(tx, ty))
					{
						addPendingSpawn(effect);
						_knife_cooldown_timer = KNIFE_COOLDOWN;
						_knife_thrown_this_cast = true;  // Oznaczamy rzut wykonany w tym caście.
					}
				}
			}
		}
		
		if (!isAnimationLocked())
		{
			if (!_knife_thrown_this_cast)
				tryThrowKnifeAtTarget();

			setAnimationSpeed(1.0f);
			_state = State::Chasing;
			playAnimation("walk");
		}
	}

	bool Bandit::tryThrowKnifeAtTarget()
	{
		if (_knife_thrown_this_cast)
			return true;

		const auto knife = getAbility(0);
		const auto target = getTarget();
		if (!knife || !target || target->isDead())
			return false;

		const float tx = target->getCenter().x;
		const float ty = target->getCenter().y;

		if (auto effect = knife->cast(tx, ty))
		{
			addPendingSpawn(effect);
			_knife_cooldown_timer = KNIFE_COOLDOWN;
			_knife_thrown_this_cast = true;
			return true;
		}

		return false;
	}

	void Bandit::handleGettingHitState(const float dt)
	{
		Entity::update(dt);
		
		if (!isAnimationLocked())
		{
			if (_pending_interrupted_knife_throw) {
				tryThrowKnifeAtTarget();
				_pending_interrupted_knife_throw = false;
			}

			_state = _state_before_hit;
			
			switch (_state)
			{
			case State::Idle:
				setAnimationSpeed(1.0f);
				playAnimation("idle");
				break;
			case State::Chasing:
			case State::Casting:
				_state = State::Chasing;
				setAnimationSpeed(1.0f);
				playAnimation("walk");
				break;
			default:
				_state = State::Chasing;
				setAnimationSpeed(1.0f);
				playAnimation("idle");
				break;
			}
		}
	}

	void Bandit::onDeathStarted()
	{
		updateMovementSound(Audio::SoundPath::Footsteps, false);
		playSoundEffect(Audio::SoundId::HumanDeath, 0.85f);
	}

} // namespace Nawia::Entity
