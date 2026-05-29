#pragma once

#include <Level.h>

namespace Nawia::World {

	class FirstLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "Wczora"; }
		[[nodiscard]] std::vector<std::string> getLocations() const override { return {"Wczora"}; }
		[[nodiscard]] std::vector<LevelLocationFile> getLocationFiles() const override;
		[[nodiscard]] std::string getDefaultInitialLocation() const override { return "Wczora"; }
	};

} // namespace Nawia::World
