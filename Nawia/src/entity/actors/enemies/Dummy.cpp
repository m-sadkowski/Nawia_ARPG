#include "Dummy.h"
#include "Ability.h"
#include "FireballAbility.h"

#include <MathUtils.h>

#include <cmath>
#include <cstdlib>

namespace Nawia::Entity {

	Dummy::Dummy(const float x, const float y, const std::shared_ptr<Texture2D>& tex, const int max_hp, Core::Map* map)
		: EnemyInterface("Dummy", x, y, tex, max_hp, map), _stay_timer(0.0f), _fireball_cooldown_timer(0.0f)
	{
		setFaction(Faction::Enemy);
		loadModel("../assets/models/dummy.glb");
		addAnimation("walk", "../assets/models/dummy_walk.glb");
		playAnimation("default");
		
		pickNewTarget();
	}

	void Dummy::update(const float dt)
	{
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
					if (auto effect = fireball->cast(_target->getX(), _target->getY()))
					{
						addPendingSpawn(std::move(effect));
						_fireball_cooldown_timer = 5.0f; // 5 seconds cooldown
					}
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
				_pos->setX(_target_x);
				_pos->setY(_target_y);
				_is_moving = false;
				_stay_timer = (rand() % 300) / 100.0f + 1.0f; // 1-4s rest

				playAnimation("default"); // idle animation
			}
			else
			{
				_pos->setX(getX() + (dx / dist) * speed * dt);
				_pos->setY(getY() + (dy / dist) * speed * dt);
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
			const float angle = static_cast<float>((rand() % 360) / 180.0f * Core::pi);
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
				const float screen_angle = std::atan2(iso_dy, iso_dx) * 180.0f / static_cast<float>(Core::pi);
				setRotation(90.0f - screen_angle);

				return;
			}
		}

		// if fail, stay
		_stay_timer = 1.0f;
		playAnimation("default");
	}

} // namespace Nawia::Entity