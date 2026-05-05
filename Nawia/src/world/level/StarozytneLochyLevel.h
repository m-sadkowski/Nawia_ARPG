#pragma once

#include <Level.h>

namespace Nawia::World {

	/**
	 * @class StarozytneLochyLevel
	 * @brief Poziom starozytnych lochow z podlokacja areny.
	 *
	 * Korzysta z `modular_terrain.glb`; spawny sa definiowane w `starozytne_lochy.json`.
	 */
	class StarozytneLochyLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "Starozytne Lochy"; }
		[[nodiscard]] std::string getSpawnFilePath() const override {
			return "assets/data/level_entities/starozytne_lochy.json";
		}
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Lochy", "Arena"};
		}
	};

} // namespace Nawia::World
