#pragma once

#include "SpawnPoint.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace Nawia::Core {
	class Engine;
	class Map;
}

namespace Nawia::World {

	/**
	 * @class SpawnManager
	 * @brief Pre-loads all entities at level start and activates them on proximity.
	 *
	 * ## Performance Strategy
	 * Instead of creating entities at runtime (which causes lag from model
	 * loading), all entities are **pre-created during level load** and added
	 * to EntityManager in a **dormant state** (invisible + frozen).
	 *
	 * On each frame, SpawnManager only checks distances and flips the
	 * dormant flag — zero allocations, zero model loads at runtime.
	 *
	 * ## Lifecycle
	 * - `loadFromJson()` — parse JSON, create ALL entities, add as dormant
	 * - `update()` — check proximity, toggle dormant → active
	 * - `reset()` — clear everything for level re-entry
	 */
	class SpawnManager {
	public:
		SpawnManager() = default;

		/**
		 * @brief Load entity definitions from JSON and pre-create all entities.
		 *
		 * All entities are created immediately via EntityFactory and added
		 * to EntityManager. Dormant state is set based on location and trigger_radius:
		 * - Entities in `initial_location` with `trigger_radius == 0` → active
		 * - Entities in `initial_location` with `trigger_radius > 0` → dormant (proximity)
		 * - Entities in other locations → always dormant (activated on location change)
		 *
		 * @param path             Path to the level_entities JSON file
		 * @param engine           Engine pointer for EntityFactory + EntityManager
		 * @param map              Current level's Map (for enemy pathfinding)
		 * @param initial_location Name of the location the player starts in
		 * @return true if file was loaded successfully
		 */
		bool loadFromJson(const std::string& path, Core::Engine* engine, 
			Core::Map* map, const std::string& initial_location);

		/**
		 * @brief Check proximity and activate dormant entities.
		 *
		 * This is extremely lightweight — just distance checks + flag toggle.
		 * No entity creation happens here.
		 */
		void update(Vector2 player_pos, const std::string& current_location);

		/**
		 * @brief Update dormant states when the player changes location.
		 * 
		 * Freezes all entities in the old location. Wakes up entities
		 * in the new location that have trigger_radius == 0.
		 */
		void updateLocationChange(const std::string& new_location);

		/**
		 * @brief Reset all spawn points and release entity references.
		 */
		void reset();

		/**
		 * @brief Get the player spawn position for a given location.
		 */
		bool getPlayerSpawn(const std::string& location_name, Vector2& out_pos) const;

	private:
		std::vector<SpawnPoint> _spawn_points;
		std::unordered_map<std::string, Vector2> _player_spawns;
	};

} // namespace Nawia::World
