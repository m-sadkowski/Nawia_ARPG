#pragma once

#include "Level.h"

#include <string>
#include <unordered_map>
#include <memory>

namespace Nawia::Core {
    class Engine;
}

namespace Nawia::World {

	/**
	 * @class LevelManager
	 * @brief Manages levels, loading, and switching
	 */
	class LevelManager {
	public:
		LevelManager() = default;
		~LevelManager();

		void registerLevel(std::shared_ptr<Level> level);
		void changeLevel(const std::string& name, Core::Engine* engine);

		[[nodiscard]] Level* getCurrentLevel() const;

	private:
		std::unordered_map<std::string, std::shared_ptr<Level>> _levels;
		std::shared_ptr<Level> _current_level;
	};

} // namespace Nawia::World
