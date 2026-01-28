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
	 * Extends Entity with map access for navigation and AI. Provides the foundation
	 * for creating enemy types with their own behavior and abilities.
	 * 
	 * ## Creating a New Enemy
	 * 
	 * 1. Create a class inheriting from `EnemyInterface`
	 * 2. Load model and animations in constructor
	 * 3. Set up collider and faction
	 * 4. Implement `update()` with AI state machine
	 * 
	 * ## Example: Goblin Enemy
	 * 
	 * ```cpp
	 * class Goblin : public EnemyInterface {
	 * public:
	 *     Goblin(float x, float y, Core::Map* map)
	 *         : EnemyInterface("Goblin", x, y, nullptr, 50, map)
	 *     {
	 *         loadModel("assets/models/goblin.glb");
	 *         addAnimation("idle", "assets/animations/goblin_idle.glb");
	 *         addAnimation("walk", "assets/animations/goblin_walk.glb");
	 *         addAnimation("attack", "assets/animations/goblin_attack.glb");
	 *         
	 *         setCollider(std::make_unique<CircleCollider>(this, 15.0f, 0.0f, -10.0f));
	 *         setFaction(Faction::Enemy);
	 *         playAnimation("idle");
	 *     }
	 * 
	 *     void update(float dt) override {
	 *         EnemyInterface::update(dt);
	 *         // Implement AI logic here (chase player, attack, retreat, etc.)
	 *     }
	 * };
	 * ```
	 * 
	 * @see Entity for base entity functionality
	 */
	class EnemyInterface : public Entity {
	public:
		/**
		 * @brief Construct a new Enemy.
		 * @param name Enemy display name
		 * @param x Starting world X position
		 * @param y Starting world Y position
		 * @param texture Fallback 2D texture (can be nullptr if using 3D model)
		 * @param max_hp Maximum health points
		 * @param map Pointer to game map for navigation
		 */
		EnemyInterface(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture, 
					int max_hp, Core::Map* map);

		/**
		 * @brief Set the target entity to track/chase.
		 * @param target Shared pointer to target entity (usually the player)
		 */
		void setTarget(const std::shared_ptr<Entity>& target) { _target = target; }

	protected:
		// Movement control
		void moveTo(float x, float y);
		void updateMovement(float dt);

		void setMovementSpeed(float speed) { _movement_speed = speed; }
		[[nodiscard]] float getMovementSpeed() const { return _movement_speed; }
	
		/**
		 * @brief Validates movement against walkability grid.
		 * Returns actual movement vector (may be reduced or zeroed if blocked).
		 * Uses "sliding" - if full movement blocked, tries X-only or Y-only.
		 */
		[[nodiscard]] Vector2 getValidatedMovement(Vector2 current_pos, Vector2 direction, float speed, float dt) const;

		// Target tracking helpers
		[[nodiscard]] float getDistanceToTarget() const;
		[[nodiscard]] Vector2 getTargetPosition() const;
		[[nodiscard]] bool hasValidTarget() const;
		
		/**
		 * @brief Standard chase behavior using A* pathfinding.
		 * Handles path recalculation timer and movement updates.
		 * @param dt Delta time
		 * @param path_recalc_interval How often to recalculate path (default 0.5s)
		 */
		void chaseTarget(float dt, float path_recalc_interval = DEFAULT_PATH_RECALC_INTERVAL);

		// Constants
		static constexpr float DEFAULT_PATH_RECALC_INTERVAL = 0.5f;

	protected:
		// Target tracking
		std::weak_ptr<Entity> _target;
		float _path_recalc_timer = 0.0f;
		
		// Movement state
		bool _is_moving;
		float _movement_speed = 2.0f;
		
		std::vector<Vector2> _path;
		float _target_x, _target_y;

		Core::Map* _map;
	};

} // namespace Nawia::Entity