#pragma once

#include "Camera.h"
#include <Entity.h>

#include <vector>
#include <memory>

namespace Nawia::Core {

	class Engine;

	class EntityManager {
	public:
		EntityManager(Engine* engine) : _engine(engine) {}
		~EntityManager() = default;

		/// Get all active entities (for rendering health bars, etc.)
		[[nodiscard]] const std::vector<std::shared_ptr<Entity::Entity>>& getEntities() const { return _active_entities; }

		// Core management
		void addEntity(std::shared_ptr<Entity::Entity> new_entity);
		void setPlayer(std::shared_ptr<Entity::Entity> player) { _player = std::move(player); }
		void clearNonPlayerEntities();

	private:
		// Game Loop methods
		void updateEntities(float delta_time);
		void renderEntities(const Camera3D& camera) const;
		void handleEntitiesCollisions() const;
		void refreshCombatTargets();

		// Input 
		[[nodiscard]] std::shared_ptr<Entity::Entity> getEntityAt(float screen_x, float screen_y, const Camera3D& camera) const;
		void updateHoverState(float screen_x, float screen_y, const Camera3D& camera);

		// For collisions
		void processAbilityCollisions() const;
		void processTriggerCollisions() const;
		void processPhysicalCollisions() const;

		// For overlap
		[[nodiscard]] bool isCollidablePhysicalEntity(const std::shared_ptr<Entity::Entity>& e) const;
		void resolveOverlap(const std::shared_ptr<Entity::Entity>& e1, const std::shared_ptr<Entity::Entity>& e2) const;
		[[nodiscard]] std::shared_ptr<Entity::Entity> findClosestCombatTarget(const std::shared_ptr<Entity::Entity>& seeker) const;
		

	private:
		Engine* _engine = nullptr;
		std::vector<std::shared_ptr<Entity::Entity>> _active_entities;
		std::shared_ptr<Entity::Entity> _player;

		friend class Engine;
	};

} // namespace Nawia::Core
