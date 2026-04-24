#pragma once

#include "Level.h"

namespace Nawia::World {

	/**
	 * @class MrocznyLasLevel
	 * @brief Dark forest level with multiple cave sub-locations.
	 *
	 * Locations:
	 *  - Las (starting area)
	 *  - Mala Jaskinia (small cave)
	 *  - Gleboka Jaskinia (deep cave — boss area)
	 *
	 * Transitions between locations will use teleport portals (NYI).
	 * Currently uses a placeholder map (demo_map/inferno.glb).
	 * Spawn definitions in assets/data/spawns/mroczny_las.json.
	 */
	class MrocznyLasLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "Mroczny Las"; }
		[[nodiscard]] std::string getSpawnFilePath() const override {
			return "../assets/data/level_entities/mroczny_las.json";
		}
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Las", "Mala Jaskinia", "Gleboka Jaskinia"};
		}
	};

} // namespace Nawia::World
