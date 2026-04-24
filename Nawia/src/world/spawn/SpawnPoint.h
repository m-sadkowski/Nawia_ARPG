#pragma once

#include <raylib.h>
#include <json.hpp>

#include <memory>
#include <string>

namespace Nawia::Entity { class Entity; }

namespace Nawia::World {

	/**
	 * @struct SpawnPoint
	 * @brief Describes a single entity spawn definition loaded from JSON.
	 *
	 * Each SpawnPoint belongs to a specific location within a level and
	 * defines what entity to spawn, where, and under what conditions.
	 *
	 * ## Activation Modes
	 * - **Immediate** (`trigger_radius == 0`): Entity becomes active as soon
	 *   as the player enters the location.
	 * - **Proximity** (`trigger_radius > 0`): Entity wakes up only when
	 *   the player comes within `trigger_radius` units of `spawn_center`.
	 *
	 * All entities are pre-created at level load time (dormant + invisible)
	 * to avoid runtime lag from model loading. Proximity only toggles
	 * the dormant flag.
	 */
	struct SpawnPoint {
		// ═══════════════════════════════════════════════════════════════
		// IDENTITY
		// ═══════════════════════════════════════════════════════════════

		std::string location;       ///< Location name this spawn belongs to (e.g. "Las")
		std::string entity_type;    ///< Factory key (e.g. "devil", "chest", "npc")
		nlohmann::json entity_data; ///< Full JSON blob passed to EntityFactory::create()

		// ═══════════════════════════════════════════════════════════════
		// PRE-CREATED ENTITY
		// ═══════════════════════════════════════════════════════════════

		std::shared_ptr<Entity::Entity> entity; ///< Pre-created entity (starts dormant)

		// ═══════════════════════════════════════════════════════════════
		// POSITIONING & TRIGGER
		// ═══════════════════════════════════════════════════════════════

		Vector2 spawn_center = {0, 0}; ///< Center point for proximity check
		float trigger_radius = 0.0f;   ///< 0 = activate immediately; >0 = distance-based
		float spawn_radius = 0.0f;     ///< Random offset radius from spawn_center

		// ═══════════════════════════════════════════════════════════════
		// STATE TRACKING
		// ═══════════════════════════════════════════════════════════════

		bool activated = false;        ///< Has this entity been woken up?
		bool respawnable = false;      ///< Can it respawn after entity dies?
		float respawn_cooldown = 0.0f; ///< Seconds before respawn (if respawnable)
		float respawn_timer = 0.0f;    ///< Current countdown timer

		/**
		 * @brief Reset state (called when re-entering a level).
		 */
		void reset() {
			activated = false;
			respawn_timer = 0.0f;
			entity.reset();
		}
	};

} // namespace Nawia::World
