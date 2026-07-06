#include "EntityNavigationSupport.h"

#include <Entity.h>
#include <Map.h>

namespace Nawia::Entity::EntityNavigationSupport {

	bool snapToNavmesh(Entity& entity, const Core::Map* map) {
		if (!map || !map->getNavMesh().isReady())
			return false;

		const Vector3 snapped = map->getNavMesh().getClosestWalkablePosition(entity.getWorldPos3D());
		entity.setX(snapped.x);
		entity.setY(snapped.z);
		entity.setAltitude(snapped.y);
		return true;
	}

} // namespace Nawia::Entity::EntityNavigationSupport
