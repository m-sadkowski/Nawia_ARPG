#pragma once

#include <Level.h>

namespace Nawia::World {

	/**
	 * @class DemoLevel
	 * @brief Poziom demonstracyjny pokazujacy glowne mechaniki gry.
	 *
	 * Zawiera kilka typow encji, spawny z `demo_level.json` oraz dwie lokacje
	 * testowe oparte o istniejace modele map.
	 */
	class DemoLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "DemoLevel"; }
		[[nodiscard]] std::string getSpawnFilePath() const override {
			return "assets/data/level_entities/demo_level.json";
		}
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Demo Arena", "Inferno"};
		}

		void changeLocation(Core::Engine* engine, const std::string& location_name) override;
	};

} // namespace Nawia::World
