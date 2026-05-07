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
		/** @brief Wczytuje startowa mape demo, oswietlenie i spawny. */
		void onEnter(Core::Engine* engine) override;

		/** @brief Zwraca nazwe poziomu demo. */
		[[nodiscard]] std::string getName() const override { return "DemoLevel"; }

		/** @brief Zwraca plik JSON ze spawnami poziomu demo. */
		[[nodiscard]] std::string getSpawnFilePath() const override {
			return "assets/data/level_entities/demo_level.json";
		}

		/** @brief Zwraca lokacje dostepne w poziomie demo. */
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Tajemniczy Las", "Lesna Dolina"};
		}

		/** @brief Przelacza geometrie mapy i deleguje teleportacje do Level. */
		void changeLocation(Core::Engine* engine, const std::string& location_name) override;
	};

} // namespace Nawia::World
