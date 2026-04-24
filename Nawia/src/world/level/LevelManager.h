#pragma once

#include "Level.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Nawia::Core {
    class Engine;
}

namespace Nawia::World {

	/**
	 * @struct LevelInfo
	 * @brief Read-only snapshot of a registered level's metadata.
	 *
	 * Used by the UI layer (LevelSelectMenu) to display level cards
	 * without needing direct access to Level objects.
	 */
	struct LevelInfo {
		std::string name;                    ///< Display name of the level
		std::vector<std::string> locations;  ///< List of location names within the level
	};

	/**
	 * @class LevelManager
	 * @brief Central registry for game levels; handles level switching.
	 *
	 * Levels are registered at startup via registerLevel(). The engine calls
	 * changeLevel() to transition between them (calls onExit on the old level,
	 * then onEnter on the new one). Per-frame hooks (handleInput, update,
	 * renderUI) are forwarded to the currently active level.
	 */
	class LevelManager {
	public:
		LevelManager() = default;
		~LevelManager();

		/**
		 * @brief Register a level so it can be selected from the UI.
		 * @param level Shared pointer to the level instance.
		 */
		void registerLevel(std::shared_ptr<Level> level);

		/**
		 * @brief Transition to a different level by name.
		 *
		 * Calls onExit() on the current level (if any) and onEnter() on the new one.
		 * @param name Exact name returned by Level::getName()
		 * @param engine Engine pointer passed to onEnter/onExit
		 */
		void changeLevel(const std::string& name, Core::Engine* engine);

		/**
		 * @brief Returns metadata for all registered levels (for UI display).
		 * @return Vector of LevelInfo structs with names and location lists.
		 */
		[[nodiscard]] std::vector<LevelInfo> getRegisteredLevelInfos() const;

		// ═══════════════════════════════════════════════════════════════════
		// PER-FRAME FORWARDING
		// ═══════════════════════════════════════════════════════════════════

		/** @brief Forward input handling to the current level. */
		void handleInput(Core::Engine* engine);

		/** @brief Forward update to the current level. */
		void update(Core::Engine* engine, float dt);

		/** @brief Forward UI rendering to the current level. */
		void renderUI(Core::Engine* engine);

		// ═══════════════════════════════════════════════════════════════════
		// QUERIES
		// ═══════════════════════════════════════════════════════════════════

		/** @brief Returns a raw pointer to the currently active level, or nullptr. */
		[[nodiscard]] Level* getCurrentLevel() const;

		/** @brief Returns current level's display name, or empty string if none. */
		[[nodiscard]] std::string getCurrentLevelName() const;

		/** @brief Returns current location name within the active level, or empty string. */
		[[nodiscard]] std::string getCurrentLocationName() const;

	private:
		std::unordered_map<std::string, std::shared_ptr<Level>> _levels;
		std::shared_ptr<Level> _current_level;
	};

} // namespace Nawia::World
