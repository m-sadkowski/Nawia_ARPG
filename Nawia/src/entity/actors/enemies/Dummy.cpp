#include "Dummy.h"
#include <MathUtils.h>
#include <cmath>
#include <cstdlib>

namespace Nawia::Entity 
{

	Dummy::Dummy(const float x, const float y, const std::shared_ptr<SDL_Texture>& tex, const int max_hp, Core::Map* map)
		: EnemyInterface(x, y, tex, max_hp, map), _stay_timer(0.0f)
	{
		pickNewTarget();
	}

	void Dummy::update(const float dt)
	{
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

	void Dummy::pickNewTarget() {
		// try 10 times to find valid spot
		for (int i = 0; i < 10; ++i) {
			const float angle = static_cast<float>((rand() % 360) / 180.0f * Core::pi);
			const float dist = static_cast<float>(rand() % 5) + 1.0f;

			const float tx = getX() + cos(angle) * dist;
			const float ty = getY() + sin(angle) * dist;

			if (_map && _map->isWalkable(static_cast<int>(tx), static_cast<int>(ty)))
			{
				_target_x = tx;
				_target_y = ty;
				_is_moving = true;
				return;
			}
		}
		// if fail, stay
		_stay_timer = 1.0f;
	}

} // namespace Nawia::Entity
