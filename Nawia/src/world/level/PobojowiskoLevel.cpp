#include "PobojowiskoLevel.h"

#include <Map.h>
#include <Engine.h>
#include <Logger.h>

namespace Nawia::World {

	void PobojowiskoLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu Pobojowisko...");

		_map = std::make_unique<Core::Map>(engine->getResourceManager());
		_map->loadMap("demo_map/inferno.glb", 1.f, {0.0f, -20.0f, 0.0f}, {0, 180, 0});

		engine->getEntityManager().clearNonPlayerEntities();

		loadSpawns(engine);
	}

} // namespace Nawia::World
