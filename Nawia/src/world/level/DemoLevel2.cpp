#include "DemoLevel2.h"

#include <Engine.h>
#include <Logger.h>

namespace Nawia::World {

	namespace {
		constexpr const char* DIABELSKI_LAS_MUSIC =
			"assets/audio/music/soundsbyamelia-baba-yaga-ritual-slavic-horror-with-hurdy-gurdy-amp-choir-422979.mp3";
		constexpr const char* LESNA_DOLINA_MUSIC =
			"assets/audio/music/soundsbyamelia-slavic-war-dance-drums-stomps-amp-war-pipes-422949.mp3";
		constexpr const char* DEMO_LIGHTING_FILE = "assets/maps/forest_lighting.json";
		constexpr float LOCATION_MUSIC_VOLUME = 0.75f;

		const std::vector<LevelLocationFile> DEMO_LOCATIONS = {
			{"Diabelski Las", "assets/data/locations/diabelski_las.json"},
			{"Lesna Dolina", "assets/data/locations/lesna_dolina.json"},
		};
	}

	void DemoLevel2::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu DemoLevel2...");

		engine->getLightingSystem().loadLightingFromJson(DEMO_LIGHTING_FILE);
		loadLocations(engine, DEMO_LOCATIONS, "Diabelski Las");
		playLocationMusic(engine, getCurrentLocationName());
	}

	void DemoLevel2::changeLocation(Core::Engine* engine, const std::string& location_name) {
		Level::changeLocation(engine, location_name);
		playLocationMusic(engine, location_name);
	}

	void DemoLevel2::playLocationMusic(Core::Engine* engine, const std::string& location_name) const {
		if (!engine)
			return;

		if (location_name == "Lesna Dolina") {
			engine->getAudioManager().playMusic(LESNA_DOLINA_MUSIC, true, LOCATION_MUSIC_VOLUME);
		} else {
			engine->getAudioManager().playMusic(DIABELSKI_LAS_MUSIC, true, LOCATION_MUSIC_VOLUME);
		}
	}

} // namespace Nawia::World
