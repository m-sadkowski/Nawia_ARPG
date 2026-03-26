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
		EnemyInterface(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture, 
					int max_hp, Core::Map* map);

		void setTarget(const std::shared_ptr<Entity>& target) { _target = target; }

	protected:
		template <typename T> friend class EnemyBuilder;
		EnemyInterface() {
			_type = EntityType::Enemy;
		}

		// Movement control (straight-line)
		void moveTo(float x, float y);
		void updateMovement(float dt);

		void setMovementSpeed(float speed) { _movement_speed = speed; }
		[[nodiscard]] float getMovementSpeed() const { return _movement_speed; }

		// Target tracking helpers
		[[nodiscard]] float getDistanceToTarget() const;
		[[nodiscard]] Vector2 getTargetPosition() const;
		[[nodiscard]] bool hasValidTarget() const;
		
		/**
		 * @brief Standard chase behavior — straight-line movement to target.
		 */
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