#include "Level.h"

#include <Engine.h>

namespace Nawia::World {

	void Level::onExit(Core::Engine* engine) {
		if (engine)
			engine->getEntityManager().clearNonPlayerEntities();
	}

} // namespace Nawia::World
