#pragma once

#include <memory>
#include <string>

namespace Nawia::Core {
    class Engine;
    class Map;
}

namespace Nawia::World {

	/**
	 * @class Level
	 * @brief Interface for a game level/scene
	 */
	class Level {
	public:
		virtual ~Level() = default;

		/// @brief Called when transitioning to this level
		virtual void onEnter(Core::Engine* engine) = 0;

		/// @brief Called when leaving this level
		virtual void onExit(Core::Engine* engine) {}

		/// @brief Returns the map owned by this level
		[[nodiscard]] virtual Core::Map* getMap() const = 0;

		/// @brief Returns the unique name of this level
		[[nodiscard]] virtual std::string getName() const = 0;
	};

} // namespace Nawia::World
