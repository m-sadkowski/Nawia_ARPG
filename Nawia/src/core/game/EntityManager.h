#pragma once
#include "Entity.h"
#include "Camera.h"

#include <vector>

namespace Nawia::Core
{

	class EntityManager
	{
	public:
		EntityManager() = default;
		~EntityManager() = default;

		void addEntity(std::shared_ptr<Entity::Entity> new_entity);
		std::shared_ptr<Entity::Entity> getEntityAt(float screen_x, float screen_y, Camera camera) const;
		void renderEntities(SDL_Renderer* renderer, Camera camera) const;
		void handleEntitiesCollisions() const;
		void updateEntities(float delta_time);

	private:
		std::vector<std::shared_ptr<Entity::Entity>> _active_entities;
	};

} // namespace Nawia::Core
