#pragma once

#include "Entity.h"

#include <memory>

namespace Nawia::Core {
	class Map;
}

namespace Nawia::Entity {

	/**
	 * @class EnemyInterface
	 * @brief Base class for all enemy entities.
	 * 
	 * Extends Entity with target tracking and movement.
	 * Movement is straight-line (pathfinding disabled for now).
	 */
	class EnemyInterface : public Entity {
	public:


		void setTarget(const std::shared_ptr<Entity>& target) { _target = target; }

	protected:
		template <typename T> friend class EnemyBuilder;
		EnemyInterface() {
			_type = EntityType::Enemy;
		}

		EnemyInterface(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture, int max_hp, Core::Map* map)
			: Entity(name, x, y, texture, max_hp), _map(map) {
			_type = EntityType::Enemy;
		}

		Core::Map* _map = nullptr;
	};

	template <typename Derived>
	class EnemyBuilder : public EntityBuilder<Derived> {
	public:
		EnemyBuilder() = default;

		Derived& setMap(Core::Map* map) {
			auto enemy_ptr = static_cast<EnemyInterface*>(this->_entity);
			enemy_ptr->_map = map;
			return this->self();
		}

		Derived& setTarget(const std::shared_ptr<Entity>& target) {
			auto enemy_ptr = static_cast<EnemyInterface*>(this->_entity);
			enemy_ptr->_target = target;
			return this->self();
		}
	};

} // namespace Nawia::Entity