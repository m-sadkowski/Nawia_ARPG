#include "BossPreloader.h"

#include <BossEnemyFactory.h>
#include <Entity.h>

namespace Nawia::Game {

	bool BossPreloader::preloadBossDefinition(
		const BossData& boss,
		Core::Engine* engine,
		std::map<std::string, std::shared_ptr<Entity::Entity>>& boss_pool,
		std::map<std::string, std::vector<std::shared_ptr<Entity::Entity>>>& minion_pools)
	{
		bool preloaded_anything = false;

		if (!boss_pool.contains(boss.id)) {
			auto boss_entity = BossEnemyFactory::create(boss.enemy_type, boss.name, boss.max_hp, engine);
			if (boss_entity) {
				boss_entity->setDormant(true);
				boss_pool[boss.id] = boss_entity;
				preloaded_anything = true;
			}
		}

		std::map<std::string, int> minion_counts;
		for (const auto& phase : boss.phases) {
			for (const auto& minion : phase.minions)
				minion_counts[minion.enemy_type] += minion.count;
		}

		for (const auto& [type, count] : minion_counts) {
			auto& pool = minion_pools[type];
			while (static_cast<int>(pool.size()) < count) {
				auto minion = BossEnemyFactory::create(type, "Minion", 60, engine);
				if (!minion)
					break;

				minion->setDormant(true);
				pool.push_back(minion);
				preloaded_anything = true;
			}
		}

		return preloaded_anything;
	}

} // namespace Nawia::Game
