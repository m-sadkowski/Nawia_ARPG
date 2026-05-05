#include "DemoLevel.h"

#include <Engine.h>
#include <Map.h>
#include <Logger.h>

namespace Nawia::World {

	namespace {
		constexpr const char* DEMO_ARENA_MAP = "forest.glb";
		constexpr float DEMO_ARENA_SCALE = 2.0f;
		constexpr const char* INFERNO_MAP = "mountain_valley.glb";
		constexpr float INFERNO_SCALE = 2.0f;
		constexpr const char* DEMO_LIGHTING_FILE = "assets/maps/forest_lighting.json";
	}

	void DemoLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu DemoLevel...");

		_map = std::make_unique<Core::Map>(engine->getResourceManager());
		_map->loadMap(DEMO_ARENA_MAP, DEMO_ARENA_SCALE);
		engine->getLightingSystem().loadLightingFromJson(DEMO_LIGHTING_FILE);

		engine->getEntityManager().clearNonPlayerEntities();

		// Wczytujemy spawny i pozycję gracza z JSON-a.
		loadSpawns(engine);
	}

	void DemoLevel::changeLocation(Core::Engine* engine, const std::string& location_name) {
		// Zmiana geometrii mapy
		if (location_name == "Demo Arena") {
			_map->loadMap(DEMO_ARENA_MAP, DEMO_ARENA_SCALE);
		} else if (location_name == "Inferno") {
			_map->loadMap(INFERNO_MAP, INFERNO_SCALE);
		}

		// Wywołaj bazową logikę (przebudzenie/zamrożenie encji, teleport gracza)
		Level::changeLocation(engine, location_name);
	}

} // namespace Nawia::World
