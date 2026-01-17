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

	protected:
		bool _is_moving;     ///< Whether the enemy is currently moving
		Core::Map* _map;    ///< Map reference for pathfinding and collision
	};

} // namespace Nawia::Entity