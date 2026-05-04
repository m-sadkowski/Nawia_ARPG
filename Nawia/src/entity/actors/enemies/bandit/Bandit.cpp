#include "Bandit.h"

#include <Ability.h>
#include <Collider.h>
#include <KnifeThrowAbility.h>
#include <Map.h>
#include <MathUtils.h>

#include <raymath.h>

namespace Nawia::Entity {

	Bandit::Bandit() {
		setScale(0.015f);
		setFaction(Faction::Enemy);

		loadModel("../assets/models/bandit_idle.glb");
		addAnimation("idle", "../assets/models/bandit_idle.glb");
		addAnimation("walk", "../assets/models/bandit_walk_backwards3.glb");
		addAnimation("throw", "../assets/models/bandit_throw.glb");
		addAnimation("death", "../assets/models/bandit_death.glb");
		addAnimation("get_hit", "../assets/models/bandit_hit.glb");
	}

	void Bandit::takeDamage(const int dmg)
	{
		Entity::takeDamage(dmg);
		
		if (isDying()) return;
		
		if (_state != State::GettingHit)
			_state_before_hit = _state;
		
		_state = State::GettingHit;
		playAnimation("get_hit", false, true, 10, true);
		setVelocity(0, 0);
	}

	void Bandit::update(const float dt)
	{
		if (isDying())
		{
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

		if (auto target = _target.lock())
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

		auto target = _target.lock();
		if (!target || target->isDead()) {
			_state = State::Idle;
			playAnimation("idle");
			setVelocity(0, 0);
			_is_moving = false;
			return;
		}

		const float dist = getDistanceToTarget();

		if (dist > VISION_RANGE * 1.5f) {
			_state = State::Idle;
			playAnimation("idle");
			setVelocity(0, 0);
			_is_moving = false;
			return;
		}

		// Sprawdzenie gotowości rzutu nożem.
		if (dist <= ATTACK_RANGE && _knife_cooldown_timer <= 0.0f) {
			if (const auto knife = getAbility(0)) {
				if (knife->isReady()) {
					_state = State::Casting;
		_knife_thrown_this_cast = false;  // Reset flagi dla nowej sekwencji rzutu.
					setAnimationSpeed(1.5f);
					playAnimation("throw", false, true);
					rotateTowards(target->getX(), target->getY());
					setVelocity(0, 0);
					_is_moving = false;
					return;
				}
			}
		}

		const Vector2 my_pos = getCenter();
		const Vector2 target_pos = target->getCenter();
		
		setMovementSpeed(SPEED);
		
		// Cel jest za blisko, więc bandyta wycofuje się z użyciem wyznaczania ścieżki.
		if (dist < MIN_DISTANCE) 
		{
			_path_recalc_timer -= dt;
			
			if (_path_recalc_timer <= 0.0f || !_is_moving || !_is_retreating) 
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
						retreat_point = { my_pos.x + std::cos(angle) * retreat_dist, my_pos.y + std::sin(angle) * retreat_dist };
						if (_map->isWalkable(retreat_point.x, retreat_point.y)) break;
					}
				}
				
				moveTo(retreat_point.x, retreat_point.y);
				_path_recalc_timer = PATH_RECALC_INTERVAL;
				_is_retreating = true;
			}
			
			playAnimation("walk");
			updateMovement(dt);
			rotateTowards(target->getX(), target->getY());  // Podczas odwrotu nadal patrzymy na cel.
		}
		// Cel jest za daleko, więc bandyta podchodzi.
		else if (dist > ATTACK_RANGE) 
		{
			_path_recalc_timer -= dt;
			
			if (_path_recalc_timer <= 0.0f || !_is_moving || _is_retreating) 
			{
				moveTo(target_pos.x, target_pos.y);
				_path_recalc_timer = PATH_RECALC_INTERVAL;
				_is_retreating = false;
			}
			
			playAnimation("walk");
			updateMovement(dt);
		}
		// Dystans jest dobry, więc bandyta stoi i celuje.
		else 
		{
			playAnimation("idle");
			_is_moving = false;
			_is_retreating = false;
		}
		
		rotateTowards(target->getX(), target->getY());
	}

	void Bandit::handleCastingState(const float dt)
	{
		Entity::update(dt);
		updateAbilities(dt);

		// Podczas animacji rzutu bandyta dalej obraca się w stronę celu.
		if (auto target = _target.lock())
		{
			rotateTowards(target->getX(), target->getY());
		}

		// Nóż rzucamy tylko raz, w konkretnej fazie animacji.
		if (_anim_frame_counter > 60 && !_knife_thrown_this_cast)
		{
			if (const auto knife = getAbility(0))
			{
				if (const auto target = _target.lock())
				{
					const float tx = target->getCenter().x;
					const float ty = target->getCenter().y;

					if (auto effect = knife->cast(tx, ty))
					{
						addPendingSpawn(std::move(effect));
						_knife_cooldown_timer = KNIFE_COOLDOWN;
						_knife_thrown_this_cast = true;  // Oznaczamy rzut wykonany w tym caście.
					}
				}
			}
		}
		
		if (!isAnimationLocked())
		{
			_state = State::Chasing;
			playAnimation("walk");
		}
	}

	void Bandit::handleGettingHitState(const float dt)
	{
		Entity::update(dt);
		
		if (!isAnimationLocked())
		{
			_state = _state_before_hit;
			
			switch (_state)
			{
			case State::Idle:
				playAnimation("idle");
				break;
			case State::Chasing:
			case State::Casting:
				
			default:
				playAnimation("idle");
				break;
			}
		}
	}

} // namespace Nawia::Entity
