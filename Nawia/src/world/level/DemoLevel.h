#pragma once

#include "Level.h"

#include <Map.h>

#include <memory>
#include <string>

namespace Nawia::World {

	/**
	 * @class DemoLevel
	 * @brief Concrete implementation of a level. Used for testing the system.
	 */
	class DemoLevel : public Level {
	public:
		DemoLevel();
		~DemoLevel() override;

		void onEnter(Core::Engine* engine) override;
		void onExit(Core::Engine* engine) override;

		[[nodiscard]] Core::Map* getMap() const override;
		[[nodiscard]] std::string getName() const override;

	private:
		std::unique_ptr<Core::Map> _map;
	};

} // namespace Nawia::World
