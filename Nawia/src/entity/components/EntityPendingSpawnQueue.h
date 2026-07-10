#pragma once

#include <memory>
#include <vector>

namespace Nawia::Entity {
	class Entity;

	class EntityPendingSpawnQueue {
	public:
		void add(std::shared_ptr<Entity> entity);
		[[nodiscard]] const std::vector<std::shared_ptr<Entity>>& pending() const { return _pending_spawns; }
		void clear() { _pending_spawns.clear(); }

	private:
		std::vector<std::shared_ptr<Entity>> _pending_spawns;
	};

}
