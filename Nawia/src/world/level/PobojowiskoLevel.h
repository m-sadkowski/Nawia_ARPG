#pragma once

#include "Level.h"

namespace Nawia::World {

	/**
	 * @class PobojowiskoLevel
	 * @brief Battlefield level with abandoned village sub-location.
	 *
	 * Locations:
	 *  - Pole Pobitewne (open battlefield — starting area)
	 *  - Opuszczona Wioska (abandoned village — exploration/loot area)
	 *
	 * Currently uses a placeholder map (demo_map/inferno.glb).
	 * Spawn definitions in assets/data/spawns/pobojowisko.json.
	 */
	class PobojowiskoLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "Pobojowisko"; }
		[[nodiscard]] std::string getSpawnFilePath() const override {
			return "../assets/data/level_entities/pobojowisko.json";
		}
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Pole Pobitewne", "Opuszczona Wioska"};
		}
	};

} // namespace Nawia::World
