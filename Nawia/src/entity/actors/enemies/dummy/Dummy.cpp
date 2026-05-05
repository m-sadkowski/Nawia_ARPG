#include "Dummy.h"

#include <Ability.h>
#include <Collider.h>
#include <FireballAbility.h>
#include <Map.h>
#include <MathUtils.h>

#include <cmath>
#include <cstdlib>

namespace Nawia::Entity {

	Dummy::Dummy(const float x, const float y, const std::shared_ptr<Texture2D>& tex, const int max_hp, Core::Map* map)
		: _stay_timer(0.0f), _fireball_cooldown_timer(0.0f)
	{
		setName("Dummy");
		setX(x);
		setY(y);
		_texture = tex;
		setMaxHp(max_hp);
		_map = map;
		_target_x = x;
		_target_y = y;

		this->setScale(0.03f);
		setFaction(Faction::Enemy);
		loadModel("assets/models/dummy_idle.glb");
		
		addAnimation("walk", "assets/models/dummy_walk.glb");
		addAnimation("cast_fireball", "assets/models/dummy_cast_fireball.glb");
		addAnimation("death", "assets/models/dummy_death.glb");
		playAnimation("default");
		
		pickNewTarget();
	}

	void Dummy::update(const float dt)
	{
		if (isDying())
		{
			Entity::update(dt);
			return;
		}

		if (_is_casting)
		{
			handleCastingState(dt);
			return;
		}

		handleActiveState(dt);
	}



	void Dummy::handleCastingState(const float dt)
	{
		Entity::update(dt);
		updateAbilities(dt);

		if (auto target = _target.lock())
		{
			_target_x = target->getX();
			_target_y = target->getY();

			rotateTowards(_target_x, _target_y);
		}
		
		if (!isAnimationLocked())
		{
			// Animacja rzutu dobiegła końca.
			if (const auto fireball = getAbility(0))
			{
				// Używamy zapamiętanego celu, a jeśli nadal istnieje, odświeżamy pozycję.
				float tx = _target_x;
				float ty = _target_y;
				
				if (auto target = _target.lock()) {
					tx = target->getCenter().x;
					ty = target->getCenter().y;
				}

				if (auto effect = fireball->cast(tx, ty))
				{
					addPendingSpawn(effect);
					_fireball_cooldown_timer = 5.0f;
				}
			}
			_is_casting = false;
			playAnimation("walk");
		}
	}

	void Dummy::handleActiveState(const float dt)
	{
		// Bazowa logika wroga aktualizuje animacje i stan encji.
		EnemyInterface::update(dt);
		updateAbilities(dt);

		if (_fireball_cooldown_timer > 0.0f)
			_fireball_cooldown_timer -= dt;

		if (auto target = _target.lock(); target && !target->isDead())
		{
			// Próba użycia pierwszej umiejętności, jeśli spełniono warunki.
			if (const auto fireball = getAbility(0))
			{
				if (_fireball_cooldown_timer <= 0.0f && fireball->isReady())
				{
					// Start sekwencji rzutu.
					_is_casting = true;
					playAnimation("cast_fireball", false, true);
					
					// Aktualizacja pozycji celu dla rzutu.
					_target_x = target->getCenter().x;
					_target_y = target->getCenter().y;

					rotateTowards(_target_x, _target_y);

					// Efekt zostanie utworzony dopiero po zakończeniu animacji.
					return; // Natychmiast zatrzymujemy dalszy ruch.
				}
			}
		}

		if (_is_moving)
		{
			const float dx = _target_x - getX();
			const float dy = _target_y - getY();
			const float dist = std::sqrt(dx * dx + dy * dy);

			constexpr float speed = 2.0f;

			if (dist < 0.1f)
			{
				_pos.x = _target_x;
				_pos.y = _target_y;
				_is_moving = false;
				_stay_timer = (rand() % 300) / 100.0f + 1.0f; // Odpoczynek 1-4 s.

				playAnimation("default"); // Animacja bezczynności.
			}
			else
			{
				_pos.x += (dx / dist) * speed * dt;
				_pos.y += (dy / dist) * speed * dt;
			}
		}
		else
		{
			_stay_timer -= dt;
			if (_stay_timer <= 0)
				pickNewTarget();
		}
	}

	void Dummy::pickNewTarget()
	{
		// Próbujemy kilka razy znaleźć poprawny punkt patrolu.
		for (int i = 0; i < 10; ++i)
		{
			const float angle = static_cast<float>((rand() % 360) / 180.0f * PI);
			const float dist = static_cast<float>(rand() % 5) + 1.0f;

			const float tx = getX() + cos(angle) * dist;
			const float ty = getY() + sin(angle) * dist;

			if (_map && _map->isWalkable(static_cast<int>(tx), static_cast<int>(ty)))
			{
				_target_x = tx;
				_target_y = ty;
				_is_moving = true;

				playAnimation("walk"); // Animacja chodzenia.

				rotateTowards(_target_x, _target_y);

				return;
			}
		}

		// Jeśli nie znaleziono punktu, zostajemy w miejscu.
		_stay_timer = 1.0f;
		playAnimation("default");
	}

} // namespace Nawia::Entity
