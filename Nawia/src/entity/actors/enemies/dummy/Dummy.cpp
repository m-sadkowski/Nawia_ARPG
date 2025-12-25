#include "Dummy.h"
#include "Ability.h"
#include "FireballAbility.h"

#include <MathUtils.h>

#include <cmath>
#include <cstdlib>
#include "Collider.h"

namespace Nawia::Entity {

	Dummy::Dummy(float x, float y, const std::shared_ptr<Texture2D>& tex, int max_hp, Core::Map* map)
		: EnemyInterface("Dummy", x, y, tex, max_hp, map), _stay_timer(0.0f), _fireball_cooldown_timer(0.0f)
	{
		this->setScale(0.03f);
		setFaction(Faction::Enemy);
		loadModel("../assets/models/dummy_idle.glb");
		
		// add Collider
		setCollider(std::make_unique<RectangleCollider>(this, 0.5f, 0.5f));
		addAnimation("walk", "../assets/models/dummy_walk.glb");
		addAnimation("cast_fireball", "../assets/models/dummy_cast_fireball.glb");
		addAnimation("death", "../assets/models/dummy_death.glb");
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
		// 1. Handle Dying State
		if (_is_dying)
		{
			Entity::update(dt); // update animation
			if (!isAnimationLocked()) // animation finished (reverted to default/idle)
			{
				_hp = 0; // now truly die
			}
			return; // skip all other logic
		}

		// 2. Handle Casting State
		if (_is_casting)
		{
			Entity::update(dt);
			updateAbilities(dt);

			if (_target)
			{
				_target_x = _target->getX();
				_target_y = _target->getY();

				// Face the target continuously
				const float dx = _target_x - getX();
				const float dy = _target_y - getY();
				const float iso_dx = (dx - dy) * (Core::TILE_WIDTH / 2.0f);
				const float iso_dy = (dx + dy) * (Core::TILE_HEIGHT / 2.0f);
				const float screen_angle = std::atan2(iso_dy, iso_dx) * 180.0f / PI;
				setRotation(90.0f - screen_angle);
			}
			
			if (!isAnimationLocked())
			{
				// cast animation finished
				if (auto fireball = getAbility(0))
				{
					// use saved target or current position if lost
					float tx = _target_x;
					float ty = _target_y;
					
					if (_target) {
						tx = _target->getX();
						ty = _target->getY();
					}

					if (auto effect = fireball->cast(tx, ty))
					{
						addPendingSpawn(std::move(effect));
						_fireball_cooldown_timer = 5.0f;
					}
				}
				_is_casting = false;
			}
			return; // skip movement while casting
		}

		// updates the base enemy logic including animations and state management
		EnemyInterface::update(dt);
		updateAbilities(dt);

		if (_fireball_cooldown_timer > 0.0f)
			_fireball_cooldown_timer -= dt;

		if (_target && !_target->isDead())
		{
			// attempts to cast the first available ability if conditions are met
			if (auto fireball = getAbility(0))
			{
				if (_fireball_cooldown_timer <= 0.0f && fireball->isReady())
				{
					// Start Casting Sequence
					_is_casting = true;
					playAnimation("cast_fireball", false, true);
					
					// Update target position for the cast
					_target_x = _target->getX();
					_target_y = _target->getY();

					// Face the target
					const float dx = _target_x - getX();
					const float dy = _target_y - getY();
					const float iso_dx = (dx - dy) * (Core::TILE_WIDTH / 2.0f);
					const float iso_dy = (dx + dy) * (Core::TILE_HEIGHT / 2.0f);
					const float screen_angle = std::atan2(iso_dy, iso_dx) * 180.0f / PI;
					setRotation(90.0f - screen_angle);

					// We do not spawn yet; waiting for animation to finish
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

				const float dx = _target_x - getX();
				const float dy = _target_y - getY();
				const float iso_dx = (dx - dy) * (Core::TILE_WIDTH / 2.0f);
				const float iso_dy = (dx + dy) * (Core::TILE_HEIGHT / 2.0f);
				const float screen_angle = std::atan2(iso_dy, iso_dx) * 180.0f / PI;
				setRotation(90.0f - screen_angle);

				return;
			}
		}

		// if fail, stay
		_stay_timer = 1.0f;
		playAnimation("default");
	}

} // namespace Nawia::Entity