#include "StarozytneLochyLevel.h"

#include <Map.h>
#include <Engine.h>
#include <Logger.h>

namespace Nawia::World {

	namespace {
		constexpr const char* MAP_FILE = "modular_terrain.glb";
		constexpr float MAP_SCALE = 1.5f;
	}

	void StarozytneLochyLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu Starozytne Lochy...");

		_map = std::make_unique<Core::Map>(engine->getResourceManager());
		_map->loadMap(MAP_FILE, MAP_SCALE);

		engine->getEntityManager().clearNonPlayerEntities();

		loadSpawns(engine);
	}

} // namespace Nawia::World
