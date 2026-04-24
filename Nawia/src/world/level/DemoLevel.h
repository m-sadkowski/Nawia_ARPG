#pragma once

#include "Level.h"

namespace Nawia::World {

	/**
	 * @class DemoLevel
	 * @brief Demo/test level showcasing core gameplay features.
	 *
	 * Contains a variety of entities (enemies, chests, NPCs, checkpoints)
	 * to demonstrate and test the game systems.
	 * Uses the "demo_map/demo.glb" map.
	 */
	class DemoLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "DemoLevel"; }
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Demo Arena"};
		}
	};

} // namespace Nawia::World
