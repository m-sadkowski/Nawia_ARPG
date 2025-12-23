#pragma once
#include "Camera.h"

#include <Entity.h>

#include <vector>
#include <memory>

namespace Nawia::Core {

	class EntityManager {
	public:
		EntityManager() = default;
		~EntityManager() = default;

	private:
		void addEntity(std::shared_ptr<Entity::Entity> new_entity);
		
		[[nodiscard]] std::shared_ptr<Entity::Entity> getEntityAt(float screen_x, float screen_y, Camera camera) const;
		
		void renderEntities(const Camera& camera) const;
		void handleEntitiesCollisions() const;
		void updateEntities(float delta_time);

		[[nodiscard]] const std::vector<std::shared_ptr<Entity::Entity>>& getEntities() const { return _active_entities; }

		std::vector<std::shared_ptr<Entity::Entity>> _active_entities;

		friend class Engine;
	};

} // namespace Nawia::Core
