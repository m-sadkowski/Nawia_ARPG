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
		void onEnter(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "Pobojowisko"; }
		[[nodiscard]] std::string getSpawnFilePath() const override {
			return "assets/data/level_entities/pobojowisko.json";
		}
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Pole Pobitewne", "Opuszczona Wioska"};
		}
	};

} // namespace Nawia::World
