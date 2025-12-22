#include "Dummy.h"

#include <MathUtils.h>

#include <cmath>
#include <cstdlib>

namespace Nawia::Entity {

	Dummy::Dummy(const float x, const float y, const std::shared_ptr<Texture2D>& tex, const int max_hp, Core::Map* map)
		: EnemyInterface(x, y, tex, max_hp, map), _stay_timer(0.0f)
	{
		_animation_component = std::make_unique<AnimationComponent>("../assets/models/dummy.glb");
		// animation_component->addAnimation("../assets/models/dummy_walk.glb")
		pickNewTarget();
	}

	void Dummy::update(const float dt)
	{
		EnemyInterface::update(dt); // update animation internal timer

		if (!_animation_component)
			return;

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

				_animation_component->playAnimation(0); // idle animation
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
		if (!_animation_component)
			return;

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

				_animation_component->playAnimation(1); // walk animation

				const float dx = _target_x - getX();
				const float dy = _target_y - getY();
				const float iso_dx = (dx - dy) * (Core::TILE_WIDTH / 2.0f);
				const float iso_dy = (dx + dy) * (Core::TILE_HEIGHT / 2.0f);
				const float screen_angle = std::atan2(iso_dy, iso_dx) * 180.0f / static_cast<float>(Core::pi);
				_animation_component->setRotation(90.0f - screen_angle);

				return;
			}
		}

		// if fail, stay
		_stay_timer = 1.0f;
		_animation_component->playAnimation(0);
	}

} // namespace Nawia::Entity