#pragma once

#include "Entity.h"

#include <memory>

namespace Nawia::Core {
	class Map;
}

namespace Nawia::Entity {

	/**
	 * @class AllyInterface
	 * @brief Base class for all ally entities.
	 * 
	 * Extends Entity with target tracking and movement.
	 */
	class AllyInterface : public Entity {
	public:
		void setTarget(const std::shared_ptr<Entity>& target) { _target = target; }

	protected:
		template <typename T> friend class AllyBuilder;
		AllyInterface() {
			_type = EntityType::Ally;
		}



		Core::Map* _map = nullptr;
	};

	template <typename Derived>
	class AllyBuilder : public EntityBuilder<Derived> {
	public:
		AllyBuilder() = default;

		Derived& setMap(Core::Map* map) {
			auto ally_ptr = static_cast<AllyInterface*>(this->_entity);
			ally_ptr->_map = map;
			return this->self();
		}

		Derived& setTarget(const std::shared_ptr<Entity>& target) {
			auto ally_ptr = static_cast<AllyInterface*>(this->_entity);
			ally_ptr->_target = target;
			return this->self();
		}
	};

} // namespace Nawia::Entity