#include "FirstLevel.h"

#include <Engine.h>
#include <Logger.h>

namespace Nawia::World {

	namespace {
		constexpr const char* FIRST_LEVEL_MUSIC =
			"assets/audio/music/soulfuljamtracks-slavic-folk-308126.mp3";
		constexpr const char* FIRST_LEVEL_LIGHTING_FILE = "assets/maps/forest_lighting.json";

		const std::vector<LevelLocationFile> FIRST_LEVEL_LOCATIONS = {
			{"Wczora", "assets/data/locations/wczora.json"},
		};
	}

	std::vector<LevelLocationFile> FirstLevel::getLocationFiles() const {
		return FIRST_LEVEL_LOCATIONS;
	}

	void FirstLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu Wczora...");
		activatePreparedLocations(engine);

		if (engine) {
			engine->getLightingSystem().loadLightingFromJson(FIRST_LEVEL_LIGHTING_FILE);
			engine->getAudioManager().playMusic(FIRST_LEVEL_MUSIC, true, 0.65f);
		}
	}

} // namespace Nawia::World
