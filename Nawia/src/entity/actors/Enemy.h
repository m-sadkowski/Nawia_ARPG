#pragma once
#include "Entity.h"

namespace Nawia::Entity
{
	class Enemy : public Entity
	{
	public:
		Enemy(float x, float y, std::shared_ptr<SDL_Texture> tex, int max_hp);

		void update(float dt) override;
	};

}