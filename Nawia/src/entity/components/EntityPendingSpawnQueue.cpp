#include "EntityPendingSpawnQueue.h"

#include <utility>

namespace Nawia::Entity {

	void EntityPendingSpawnQueue::add(std::shared_ptr<Entity> entity)
	{
		if (entity)
			_pending_spawns.push_back(std::move(entity));
	}

}
