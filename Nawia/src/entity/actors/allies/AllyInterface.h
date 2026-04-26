#pragma once

#include "AllyBrain.h"
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
		void setBrain(const std::shared_ptr<AllyBrain>& brain) { _brain = brain; }
		[[nodiscard]] std::shared_ptr<AllyBrain> getBrain() const { return _brain; }

	protected:
		template <typename T> friend class AllyBuilder;
		AllyInterface() {
			_type = EntityType::Ally;
		}

		AllyInterface(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture, int max_hp, Core::Map* map)
			: Entity(name, x, y, texture, max_hp), _map(map) {
			_type = EntityType::Ally;
		}

		Core::Map* _map = nullptr;
		std::shared_ptr<AllyBrain> _brain = nullptr;
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

		Derived& setBrain(const std::shared_ptr<AllyBrain>& brain) {
			auto ally_ptr = static_cast<AllyInterface*>(this->_entity);
			ally_ptr->_brain = brain;
			return this->self();
		}
	};

} // namespace Nawia::Entity
