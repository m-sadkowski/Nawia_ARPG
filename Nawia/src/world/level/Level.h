#pragma once

#include <Map.h>
#include <SpawnManager.h>

#include <memory>
#include <string>
#include <vector>

namespace Nawia::Core {
    class Engine;
}

namespace Nawia::World {

	/**
	 * @class Level
	 * @brief Abstract base class for all game levels/scenes.
	 *
	 * A Level represents a playable stage in the game. Each level owns a Map
	 * (the 3D world geometry) and defines one or more named locations within it.
	 * Locations are logical sub-areas of a level (e.g. a forest level might
	 * contain "Las", "Mala Jaskinia", "Gleboka Jaskinia"). Transitioning between
	 * locations within a level will be handled by a teleport system (NYI).
	 *
	 * Subclasses must implement:
	 *  - onEnter()          — load map, set player position
	 *  - getName()          — return a unique human-readable level name
	 *  - getSpawnFilePath() — return path to the JSON spawn definitions file
	 *
	 * Subclasses may override:
	 *  - onExit()       — custom cleanup (default clears entities + resets spawns)
	 *  - getLocations() — list of location names (default: single "Domyslna")
	 *  - handleInput(), update(), renderUI() — per-level logic hooks
	 *
	 * Entity spawning is handled by SpawnManager, which reads JSON spawn
	 * definitions and creates entities via EntityFactory. The default
	 * update() implementation drives the SpawnManager each frame.
	 */
	class Level {
	public:
		virtual ~Level() = default;

		// ═══════════════════════════════════════════════════════════════════
		// LIFECYCLE
		// ═══════════════════════════════════════════════════════════════════

		/**
		 * @brief Called when the player enters this level.
		 *
		 * Implementations should: load the map into _map, clear old entities,
		 * reset the player position. Entity spawning is handled automatically
		 * by SpawnManager (call loadSpawns() in your onEnter).
		 * @param engine Pointer to the game engine (for accessing subsystems)
		 */
		virtual void onEnter(Core::Engine* engine) = 0;

		/**
		 * @brief Called when the player leaves this level.
		 *
		 * Default implementation clears all non-player entities.
		 * Override to add custom teardown logic.
		 * @param engine Pointer to the game engine
		 */
		virtual void onExit(Core::Engine* engine);

		// ═══════════════════════════════════════════════════════════════════
		// PER-FRAME HOOKS (optional overrides)
		// ═══════════════════════════════════════════════════════════════════

		/** @brief Level-specific input handling. Called each frame during Playing state. */
		virtual void handleInput(Core::Engine* engine) {}

		/**
		 * @brief Level-specific update logic. Called each frame during Playing state.
		 *
		 * Default implementation drives the SpawnManager (proximity checks).
		 * Override in subclasses for additional logic, but call Level::update()
		 * to keep spawn processing active.
		 */
		virtual void update(Core::Engine* engine, float dt);

		/** @brief Level-specific 2D UI rendering. Called after the 3D scene is drawn. */
		virtual void renderUI(Core::Engine* engine) {}

		// ═══════════════════════════════════════════════════════════════════
		// MAP
		// ═══════════════════════════════════════════════════════════════════

		/**
		 * @brief Returns the map (3D world geometry) owned by this level.
		 * @return Raw pointer to the Map, or nullptr if the level hasn't been entered yet.
		 */
		[[nodiscard]] Core::Map* getMap() const { return _map.get(); }

		// ═══════════════════════════════════════════════════════════════════
		// IDENTITY & LOCATIONS
		// ═══════════════════════════════════════════════════════════════════

		/**
		 * @brief Returns the unique display name of this level (e.g. "Mroczny Las").
		 *
		 * Used as a key in LevelManager and displayed in the UI.
		 */
		[[nodiscard]] virtual std::string getName() const = 0;

		/**
		 * @brief Returns the list of location names within this level.
		 *
		 * Locations are logical sub-areas. Override in subclasses to define
		 * level-specific locations. Default returns a single "Domyslna".
		 */
		[[nodiscard]] virtual std::vector<std::string> getLocations() const { return {"Domyslna"}; }

		// ═══════════════════════════════════════════════════════════════════
		// SPAWN SYSTEM
		// ═══════════════════════════════════════════════════════════════════

		/**
		 * @brief Returns the relative path to this level's JSON spawn file.
		 *
		 * Each level must provide a path to its spawn definitions file.
		 * Example: "../assets/data/spawns/demo_level.json"
		 */
		[[nodiscard]] virtual std::string getSpawnFilePath() const = 0;

		/** @brief Returns the SpawnManager for this level. */
		[[nodiscard]] SpawnManager& getSpawnManager() { return _spawn_manager; }

		/**
		 * @brief Returns the name of the currently active location.
		 *
		 * Resolved from getLocations() using _current_location_index.
		 * Falls back to the first location if the index is out of range.
		 */
		[[nodiscard]] std::string getCurrentLocationName() const {
			const auto locs = getLocations();
			if (_current_location_index < locs.size())
				return locs[_current_location_index];
			return locs.empty() ? "" : locs[0];
		}

		/**
		 * @brief Sets the active location by index.
		 * @param index Zero-based index into the getLocations() list.
		 * @note For future teleport system — not yet called by game logic.
		 */
		void setCurrentLocationIndex(size_t index) { _current_location_index = index; }

		/** @brief Returns the current location index. */
		[[nodiscard]] size_t getCurrentLocationIndex() const { return _current_location_index; }

	protected:
		/**
		 * @brief Load spawn definitions and set player position from JSON.
		 *
		 * Call this at the end of your onEnter() implementation.
		 * It loads the spawn file, positions the player, and resets spawn state.
		 */
		void loadSpawns(Core::Engine* engine);

		std::unique_ptr<Core::Map> _map;     ///< 3D world geometry for this level
		size_t _current_location_index = 0;  ///< Index into getLocations()
		SpawnManager _spawn_manager;         ///< Manages entity spawning from JSON
	};

} // namespace Nawia::World

