#include "DemoLevel.h"

#include <Engine.h>
#include <Map.h>
#include <Logger.h>

namespace Nawia::World {

	void DemoLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu DemoLevel...");

		_map = std::make_unique<Core::Map>(engine->getResourceManager());
		_map->loadMap("demo_map/demo.glb", 2.0f);

		engine->getEntityManager().clearNonPlayerEntities();

		// Load entity spawns and player position from JSON
		loadSpawns(engine);
	}

	void DemoLevel::changeLocation(Core::Engine* engine, const std::string& location_name) {
		// Zmiana geometrii mapy
		if (location_name == "Demo Arena") {
			_map->loadMap("demo_map/demo.glb", 2.0f);
		} else if (location_name == "Inferno") {
			_map->loadMap("demo_map/inferno.glb", 2.0f);
		}

		// Wywołaj bazową logikę (przebudzenie/zamrożenie encji, teleport gracza)
		Level::changeLocation(engine, location_name);
	}

} // namespace Nawia::World
