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
	class EnemyInterface;
	class Entity;
}

namespace Nawia::Game {

	class BossMinionSpawner {
	public:
		static void spawn(
			const std::vector<MinionSpawnInfo>& minions,
			const std::shared_ptr<Entity::EnemyInterface>& active_boss_entity,
			std::map<std::string, std::vector<std::shared_ptr<Entity::Entity>>>& minion_pools,
			std::vector<std::shared_ptr<Entity::Entity>>& active_minions,
			Core::Engine* engine);

		static void remove(std::vector<std::shared_ptr<Entity::Entity>>& active_minions);
	};

} // namespace Nawia::Game
