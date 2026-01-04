#pragma once
#include "Entity.h"

#include <memory>

namespace Nawia::Core {
	class Map;
}

namespace Nawia::Entity {

	class EnemyInterface : public Entity {
	public:
		EnemyInterface(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture, 
						int max_hp, Core::Map* map);

	protected:
		bool _is_moving;
		Core::Map* _map;
	};

} // namespace Nawia::Entity