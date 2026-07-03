#pragma once

#include <Level.h>

namespace Nawia::World {

	/**
	 * @brief Clean test arena for the Siewca Chaosu dragon boss on the Przedsionek Nawii map.
	 */
	class NawiaArenaLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;
		void onNewGameStarted(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "Przedsionek Nawii"; }
		[[nodiscard]] std::vector<std::string> getLocations() const override { return {"Przedsionek Nawii"}; }
		[[nodiscard]] std::vector<LevelLocationFile> getLocationFiles() const override;
		[[nodiscard]] std::string getDefaultInitialLocation() const override { return "Przedsionek Nawii"; }
	};

} // namespace Nawia::World
