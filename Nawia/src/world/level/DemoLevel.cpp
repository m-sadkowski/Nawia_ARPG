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

} // namespace Nawia::World
