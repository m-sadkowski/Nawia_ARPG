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

		/// Get all active entities (for rendering health bars, etc.)
		[[nodiscard]] const std::vector<std::shared_ptr<Entity::Entity>>& getEntities() const { return _active_entities; }

	private:
		// Core management
		void addEntity(std::shared_ptr<Entity::Entity> new_entity);
		void setPlayer(std::shared_ptr<Entity::Entity> player) { _player = std::move(player); }

		// Game Loop methods
		void updateEntities(float delta_time);
		void renderEntities(const Camera& camera) const;
		void handleEntitiesCollisions() const;

		// Input 
		[[nodiscard]] std::shared_ptr<Entity::Entity> getEntityAt(float screen_x, float screen_y, Camera camera) const;
		void updateHoverState(float screen_x, float screen_y, const Camera& camera);

		// For collisions
		void processAbilityCollisions() const;
		void processTriggerCollisions() const;
		void processPhysicalCollisions() const;

		// For overlap
		[[nodiscard]] bool isCollidablePhysicalEntity(const std::shared_ptr<Entity::Entity>& e) const;
		void resolveOverlap(const std::shared_ptr<Entity::Entity>& e1, const std::shared_ptr<Entity::Entity>& e2) const;
		

	private:

		std::vector<std::shared_ptr<Entity::Entity>> _active_entities;
		std::shared_ptr<Entity::Entity> _player;

		friend class Engine;
	};

} // namespace Nawia::Core