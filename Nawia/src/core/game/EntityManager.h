#pragma once
#include "Entity.h"
#include "Camera.h"

#include <vector>

#include "Player.h"

namespace Nawia::Core
{

	class EntityManager
	{
		friend class Engine;

	public:

		EntityManager() = default;
		~EntityManager() = default;

		
	private:
		std::shared_ptr<Entity::Player> _player;
		std::vector<std::shared_ptr<Entity::Entity>> _active_entities;
		void setPlayer(std::shared_ptr<Entity::Player> new_player);
		void addEntity(std::shared_ptr<Entity::Entity> new_entity);
		[[nodiscard]] std::shared_ptr<Entity::Entity> getEntityAt(float screen_x, float screen_y, Camera camera) const;
		void renderEntities(SDL_Renderer* renderer, const Camera& camera) const;
		void handleEntitiesCollisions() const;
		void updateEntities(float delta_time);
		
	};

} // namespace Nawia::Core
