#include "DemoLevel.h"

#include <Engine.h>
#include <Logger.h>
#include <Map.h>

namespace Nawia::World {

	namespace {
		constexpr const char* MYSTERIOUS_FOREST_MAP = "forest.glb";
		constexpr float MYSTERIOUS_FOREST_SCALE = 2.0f;
		constexpr const char* MYSTERIOUS_FOREST_MUSIC =
			"assets/audio/music/soundsbyamelia-baba-yaga-ritual-slavic-horror-with-hurdy-gurdy-amp-choir-422979.mp3";
		constexpr const char* FOREST_VALLEY_MAP = "mountain_valley.glb";
		constexpr float FOREST_VALLEY_SCALE = 2.0f;
		constexpr const char* FOREST_VALLEY_MUSIC =
			"assets/audio/music/soundsbyamelia-slavic-war-dance-drums-stomps-amp-war-pipes-422949.mp3";
		constexpr const char* DEMO_LIGHTING_FILE = "assets/maps/forest_lighting.json";
		constexpr float LOCATION_MUSIC_VOLUME = 0.75f;
	}

	void DemoLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu DemoLevel...");

		_map = std::make_unique<Core::Map>(engine->getResourceManager());
		_map->loadMap(MYSTERIOUS_FOREST_MAP, MYSTERIOUS_FOREST_SCALE);
		applyNavMeshSettingsFromJson(getCurrentLocationName());
		engine->getLightingSystem().loadLightingFromJson(DEMO_LIGHTING_FILE);

		engine->getEntityManager().clearNonPlayerEntities();

		// Wczytuje spawny i pozycje gracza z JSON-a.
		loadSpawns(engine);

		engine->getAudioManager().playMusic(MYSTERIOUS_FOREST_MUSIC, true, LOCATION_MUSIC_VOLUME);
	}

	void DemoLevel::changeLocation(Core::Engine* engine, const std::string& location_name) {
		// Zmienia geometrie mapy przed przeniesieniem gracza.
		if (location_name == "Tajemniczy Las") {
			_map->loadMap(MYSTERIOUS_FOREST_MAP, MYSTERIOUS_FOREST_SCALE);
			applyNavMeshSettingsFromJson(location_name);
			engine->getAudioManager().playMusic(MYSTERIOUS_FOREST_MUSIC, true, LOCATION_MUSIC_VOLUME);
		} else if (location_name == "Lesna Dolina") {
			_map->loadMap(FOREST_VALLEY_MAP, FOREST_VALLEY_SCALE);
			applyNavMeshSettingsFromJson(location_name);
			engine->getAudioManager().playMusic(FOREST_VALLEY_MUSIC, true, LOCATION_MUSIC_VOLUME);
		}

		// Bazowa logika budzi/zamraza encje i teleportuje gracza.
		Level::changeLocation(engine, location_name);
	}

} // namespace Nawia::World
