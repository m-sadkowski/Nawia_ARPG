#include "Enemy.h"

namespace Nawia::Entity
{
	Enemy::Enemy(float x, float y, std::shared_ptr<SDL_Texture> tex, int max_hp)
		: Entity(x, y, tex, max_hp)
	{
		
	}

	void Enemy::update(float dt)
	{
		// movement ai
	}
}
