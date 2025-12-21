#pragma once
#include "Entity.h"

namespace Nawia::Core
{
	class Map;
}

namespace Nawia::Entity
{

	class EnemyInterface : public Entity
	{
	public:
		EnemyInterface(float x, float y, const std::shared_ptr<SDL_Texture>& tex, int max_hp, Core::Map* map);

	protected:
		bool _is_moving;
		Core::Map* _map;

	};

}