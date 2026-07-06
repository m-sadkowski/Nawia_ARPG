#pragma once

#include <raylib.h>

namespace Nawia::Core {
	class Map;
}

namespace Nawia::Entity {
	class Entity;
}

namespace Nawia::Entity::EntityNavigationSupport {

	bool snapToNavmesh(Entity& entity, const Core::Map* map);

} // namespace Nawia::Entity::EntityNavigationSupport
