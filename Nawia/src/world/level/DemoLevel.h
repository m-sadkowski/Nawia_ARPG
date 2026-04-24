#pragma once

#include "Level.h"

namespace Nawia::World {

	/**
	 * @class DemoLevel
	 * @brief Demo/test level showcasing core gameplay features.
	 *
	 * Contains a variety of entities (enemies, chests, NPCs, checkpoints)
	 * defined in assets/data/spawns/demo_level.json.
	 * Uses the "demo_map/demo.glb" map.
	 */
	class DemoLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "DemoLevel"; }
		[[nodiscard]] std::string getSpawnFilePath() const override {
			return "../assets/data/level_entities/demo_level.json";
		}
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Demo Arena", "Inferno"};
		}

		void changeLocation(Core::Engine* engine, const std::string& location_name) override;
	};

} // namespace Nawia::World
