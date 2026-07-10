#include "EntityFactory.h"
#include "EntityFactoryCommon.h"

#include <Logger.h>

#include <unordered_map>

namespace Nawia::World {

	std::shared_ptr<Entity::Entity> EntityFactory::create(
		const std::string& type,
		const nlohmann::json& data,
		Core::Engine* engine,
		Core::Map* map)
	{
		using EntityFactoryDetail::CreatorFn;

		static const std::unordered_map<std::string, CreatorFn> creators = {
			{"devil", &EntityFactory::createDevil},
			{"rift_binder", &EntityFactory::createRiftBinder},
			{"dragon", &EntityFactory::createRiftBinder},
			{"witch", &EntityFactory::createWitch},
			{"bandit", &EntityFactory::createBandit},
			{"walking_dead", &EntityFactory::createWalkingDead},
			{"frog", &EntityFactory::createFrog},
			{"worm", &EntityFactory::createWorm},
			{"spider", &EntityFactory::createSpider},
			{"mini_mushroom_infected", &EntityFactory::createMiniMushroomInfected},
			{"friend", &EntityFactory::createFriend},
			{"chest", &EntityFactory::createChest},
			{"npc", &EntityFactory::createNPC},
			{"mini_mushroom_prop", &EntityFactory::createMiniMushroomProp},
			{"static_object", &EntityFactory::createStaticObject},
			{"checkpoint", &EntityFactory::createCheckpoint},
			{"checkpoint_mushroom_npc", &EntityFactory::createMushroomWaypoint},
			{"story_anchor", &EntityFactory::createStoryAnchor},
			{"herbalist_hub", &EntityFactory::createHerbalistHub},
			{"teleport", &EntityFactory::createTeleport},
			{"boss_trigger", &EntityFactory::createBossTrigger},
			{"story_trigger", &EntityFactory::createStoryTrigger}
		};

		if (const auto creator = creators.find(type); creator != creators.end())
			return creator->second(data, {engine, map});

		Core::Logger::errorLog("EntityFactory: nieznany typ encji: " + type);
		return nullptr;
	}

} // namespace Nawia::World
