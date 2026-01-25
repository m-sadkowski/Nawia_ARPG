#include "Dummy.h"
#include "Ability.h"
#include "FireballAbility.h"

#include <MathUtils.h>

#include <cmath>
#include <cstdlib>
#include "Collider.h"

namespace Nawia::Entity {

	Dummy::Dummy(const float x, const float y, const std::shared_ptr<Texture2D>& tex, const int max_hp, Core::Map* map)
		: EnemyInterface("Dummy", x, y, tex, max_hp, map), _stay_timer(0.0f), _fireball_cooldown_timer(0.0f)
	{
		this->setScale(0.01f);
		setFaction(Faction::Enemy);
		loadModel("../assets/models/test5.glb");
		
		// add collider
		setCollider(std::make_unique<RectangleCollider>(this, 0.3f, 0.8f, -2.1f, -1.f));
		addAnimation("walk", "../assets/models/test5.glb");
		addAnimation("cast_fireball", "../assets/models/test5.glb");
		addAnimation("death", "../assets/models/test5.glb");
		playAnimation("default");
		
		pickNewTarget();
	}

	void Dummy::takeDamage(const int dmg)
	{
		if (_is_dying) return;

		if (_hp - dmg <= 0)
		{
			_hp = 1; // keep alive for animation
			_is_dying = true;
			playAnimation("death", false, true); // true = lock movement
			setFaction(Faction::None); // prevent further targeting
		}
		else
		{
			Entity::takeDamage(dmg);
		}
	}

	void Dummy::update(const float dt)
	{
		if (_is_dying)
		{
			handleDyingState(dt);
			return;
		}

		if (_is_casting)
		{
			handleCastingState(dt);
			return;
		}

		handleActiveState(dt);
	}

	void Dummy::handleDyingState(const float dt)
	{
		Entity::update(dt); // update animation
		if (!isAnimationLocked()) // animation finished (reverted to default/idle)
		{
			_hp = 0; // now truly die
		}
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
			// cast animation finished
			if (const auto fireball = getAbility(0))
			{
				// use saved target or current position if lost
				float tx = _target_x;
				float ty = _target_y;
				
				if (auto target = _target.lock()) {
					tx = target->getCenter().x;
					ty = target->getCenter().y;
				}

				if (auto effect = fireball->cast(tx, ty))
				{
					addPendingSpawn(std::move(effect));
					_fireball_cooldown_timer = 5.0f;
				}
			}
			_is_casting = false;
			playAnimation("walk");
		}
	}

	void Dummy::handleActiveState(const float dt)
	{
		// updates the base enemy logic including animations and state management
		EnemyInterface::update(dt);
		updateAbilities(dt);

		if (_fireball_cooldown_timer > 0.0f)
			_fireball_cooldown_timer -= dt;

		if (auto target = _target.lock(); target && !target->isDead())
		{
			// attempts to cast the first available ability if conditions are met
			if (const auto fireball = getAbility(0))
			{
				if (_fireball_cooldown_timer <= 0.0f && fireball->isReady())
				{
					// start Casting Sequence
					_is_casting = true;
					playAnimation("cast_fireball", false, true);
					
					// update target position for the cast
					_target_x = target->getCenter().x;
					_target_y = target->getCenter().y;

					rotateTowards(_target_x, _target_y);

					// we do not spawn yet; waiting for animation to finish
					return; // Stop moving immediately
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
				_stay_timer = (rand() % 300) / 100.0f + 1.0f; // 1-4s rest

				playAnimation("default"); // idle animation
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
		// try 10 times to find valid spot
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

				playAnimation("walk"); // walk animation

				rotateTowards(_target_x, _target_y);

				return;
			}
		}

		// if fail, stay
		_stay_timer = 1.0f;
		playAnimation("default");
	}

} // namespace Nawia::Entity