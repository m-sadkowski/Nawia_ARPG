#pragma once

#include <Level.h>

namespace Nawia::World {

	/**
	 * @class MrocznyLasLevel
	 * @brief Poziom mrocznego lasu z kilkoma podlokacjami.
	 *
	 * Korzysta z `forest.glb`; spawny sa definiowane w `mroczny_las.json`.
	 */
	class MrocznyLasLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "Mroczny Las"; }
		[[nodiscard]] std::string getSpawnFilePath() const override {
			return "assets/data/level_entities/mroczny_las.json";
		}
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Las", "Mala Jaskinia", "Gleboka Jaskinia"};
		}
	};

} // namespace Nawia::World
