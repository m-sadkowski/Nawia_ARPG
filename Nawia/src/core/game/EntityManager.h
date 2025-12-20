#pragma once
#include "Entity.h"
#include "Camera.h"

#include <vector>

namespace Nawia::Core
{

	class EntityManager
	{
		friend class Engine;

	public:
		EntityManager() = default;
		~EntityManager() = default;

	private:
		std::vector<std::shared_ptr<Entity::Entity>> _active_entities;
		void addEntity(std::shared_ptr<Entity::Entity> new_entity);
		[[nodiscard]] std::shared_ptr<Entity::Entity> getEntityAt(float screen_x, float screen_y, Camera camera) const;
		void renderEntities(SDL_Renderer* renderer, Camera camera) const;
		void handleEntitiesCollisions() const;
		void updateEntities(float delta_time);
	};

} // namespace Nawia::Core
