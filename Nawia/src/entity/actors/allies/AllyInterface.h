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

		// Movement control
		void moveTo(float x, float y);
		void updateMovement(float dt);

		void setMovementSpeed(float speed) { _movement_speed = speed; }
		[[nodiscard]] float getMovementSpeed() const { return _movement_speed; }

		// Target tracking helpers
		[[nodiscard]] float getDistanceToTarget() const;
		[[nodiscard]] Vector2 getTargetPosition() const;
		[[nodiscard]] bool hasValidTarget() const;
		
		void chaseTarget(float dt, float path_recalc_interval = DEFAULT_PATH_RECALC_INTERVAL);

		// Constants
		static constexpr float DEFAULT_PATH_RECALC_INTERVAL = 0.5f;

		// Target tracking
		std::weak_ptr<Entity> _target;
		float _path_recalc_timer = 0.0f;
		
		// Movement state
		bool _is_moving = false;
		float _movement_speed = 2.0f;
		
		float _target_x = 0.0f, _target_y = 0.0f;

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