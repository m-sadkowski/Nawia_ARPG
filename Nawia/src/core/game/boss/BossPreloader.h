#pragma once

#include <BossTypes.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Nawia::Core {
	class Engine;
}

namespace Nawia::Entity {
	class Entity;
}

namespace Nawia::Game {

	class BossPreloader {
	public:
		static bool preloadBossDefinition(
			const BossData& boss,
			Core::Engine* engine,
			std::map<std::string, std::shared_ptr<Entity::Entity>>& boss_pool,
			std::map<std::string, std::vector<std::shared_ptr<Entity::Entity>>>& minion_pools);
	};

} // namespace Nawia::Game
