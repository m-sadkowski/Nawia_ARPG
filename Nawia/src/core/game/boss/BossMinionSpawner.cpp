#include "BossMinionSpawner.h"

#include <ActorInterface.h>
#include <BossEnemyFactory.h>
#include <Engine.h>
#include <EnemyInterface.h>
#include <Entity.h>
#include <Logger.h>
#include <Map.h>

#include <cmath>

namespace Nawia::Game {

	void BossMinionSpawner::spawn(
		const std::vector<MinionSpawnInfo>& minions,
		const std::shared_ptr<Entity::EnemyInterface>& active_boss_entity,
		std::map<std::string, std::vector<std::shared_ptr<Entity::Entity>>>& minion_pools,
		std::vector<std::shared_ptr<Entity::Entity>>& active_minions,
		Core::Engine* engine)
	{
		if (!active_boss_entity || !engine)
			return;

		for (const auto& info : minions) {
			for (int i = 0; i < info.count; ++i) {
				const float angle = (2.0f * 3.14159f / static_cast<float>(info.count)) * static_cast<float>(i);
				Vector2 spawn_pos = {
					active_boss_entity->getX() + info.offset_x * std::cos(angle),
					active_boss_entity->getY() + info.offset_y * std::sin(angle)
				};

				std::shared_ptr<Entity::Entity> minion = nullptr;

				if (minion_pools.count(info.enemy_type) && !minion_pools[info.enemy_type].empty()) {
					minion = minion_pools[info.enemy_type].back();
					minion_pools[info.enemy_type].pop_back();
					minion->setX(spawn_pos.x);
					minion->setY(spawn_pos.y);
					minion->setMaxHp(info.hp);
					minion->setDormant(false);
				} else {
					minion = BossEnemyFactory::create(info.enemy_type, "Minion", info.hp, engine);
					if (minion) {
						minion->setX(spawn_pos.x);
						minion->setY(spawn_pos.y);
					}
				}

				if (!minion)
					continue;

				if (auto actor = std::dynamic_pointer_cast<Entity::ActorInterface>(minion))
					actor->setMap(engine->getCurrentMap());

				auto* map = engine->getCurrentMap();
				if (map && map->getNavMesh().isReady()) {
					const Vector3 snapped_position = map->getNavMesh().getClosestWalkablePosition(
						{minion->getX(), active_boss_entity->getAltitude(), minion->getY()});
					minion->setX(snapped_position.x);
					minion->setY(snapped_position.z);
					minion->setAltitude(snapped_position.y);
				}

				active_minions.push_back(minion);
				engine->getEntityManager().addEntity(minion);
			}
		}

		Core::Logger::debugLog("BossManager: Przywolano " + std::to_string(active_minions.size()) + " minionow.");
	}

	void BossMinionSpawner::remove(std::vector<std::shared_ptr<Entity::Entity>>& active_minions)
	{
		for (auto& minion : active_minions) {
			if (minion && !minion->isDead())
				minion->die();
		}
		active_minions.clear();
		Core::Logger::debugLog("BossManager: Miniony usuniete.");
	}

} // namespace Nawia::Game
