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
		// Core management
		void addEntity(std::shared_ptr<Entity::Entity> new_entity);
		void setPlayer(std::shared_ptr<Entity::Entity> player) { _player = std::move(player); }

		// Game Loop methods
		void updateEntities(float delta_time);
		void renderEntities(const Camera& camera) const;
		void handleEntitiesCollisions();

		// Input 
		[[nodiscard]] std::shared_ptr<Entity::Entity> getEntityAt(float screen_x, float screen_y, Camera camera) const;

		// Getters
		[[nodiscard]] const std::vector<std::shared_ptr<Entity::Entity>>& getEntities() const { return _active_entities; }

		// For collisions
		void processAbilityCollisions();
		void processTriggerCollisions();
		void processPhysicalCollisions();

		// For overlap
		bool isCollidablePhysicalEntity(const std::shared_ptr<Entity::Entity>& e) const;
		void resolveOverlap(std::shared_ptr<Entity::Entity>& e1, std::shared_ptr<Entity::Entity>& e2) const;
		

	private:

		std::vector<std::shared_ptr<Entity::Entity>> _active_entities;
		std::shared_ptr<Entity::Entity> _player;

		friend class Engine;
	};

} // namespace Nawia::Core