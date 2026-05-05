#pragma once

#include <Level.h>

namespace Nawia::World {

	/**
	 * @class PobojowiskoLevel
	 * @brief Poziom pobojowiska z podlokacja opuszczonej wioski.
	 *
	 * Korzysta z `rocky-forest.glb`; spawny sa definiowane w `pobojowisko.json`.
	 */
	class PobojowiskoLevel : public Level {
	public:
		/** @brief Wczytuje mape pobojowiska i spawny lokacji. */
		void onEnter(Core::Engine* engine) override;

		/** @brief Zwraca nazwe poziomu. */
		[[nodiscard]] std::string getName() const override { return "Pobojowisko"; }

		/** @brief Zwraca plik JSON ze spawnami pobojowiska. */
		[[nodiscard]] std::string getSpawnFilePath() const override {
			return "assets/data/level_entities/pobojowisko.json";
		}

		/** @brief Zwraca lokacje dostepne na pobojowisku. */
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Pole Pobitewne", "Opuszczona Wioska"};
		}
	};

} // namespace Nawia::World
