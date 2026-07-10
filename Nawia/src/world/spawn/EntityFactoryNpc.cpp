#include "EntityFactory.h"
#include "EntityFactoryCommon.h"

#include <Cat.h>
#include <CemeterySurvivorGroupNpc.h>
#include <Engine.h>
#include <ForestLostGroupNpc.h>
#include <GenericStoryNpc.h>
#include <Logger.h>
#include <MushroomNpc.h>
#include <SzeptuchaNpc.h>
#include <VillageHeadNpc.h>
#include <WandaCorpseNpc.h>

#include <unordered_map>

namespace Nawia::World {

	namespace {

		std::shared_ptr<Entity::Entity> createCatNpc(
			const nlohmann::json& data,
			const SpawnContext& context,
			const EntityFactoryDetail::SpawnBasics& basics)
		{
			auto* engine = context.engine;
			auto cat = std::make_shared<Entity::Cat>(
				basics.name,
				basics.position.x,
				basics.position.y,
				nullptr);
			cat->setAudioManager(&engine->getAudioManager());

			engine->getDialogueManager().createCatDialogue(engine, cat.get());

			if (data.contains("loottable")) {
				const std::string loottable_name = data["loottable"].get<std::string>();
				auto& loottable = engine->getLoottable();

				cat->initializeInventory(
					loottable,
					Item::parseLoottableType(loottable_name, Item::LOOTTABLE_TYPE::CAT)
				);
			}

			EntityFactoryDetail::addItemsFromJson(data, engine, [&](const std::shared_ptr<Item::Item>& item) {
				cat->addItem(item);
			});
			if (auto key = engine->getItemDatabase().createItem(5))
				cat->addItem(key);

			return cat;
		}

		std::shared_ptr<Entity::Entity> createMushroomNpc(
			const nlohmann::json& data,
			const SpawnContext& context,
			const EntityFactoryDetail::SpawnBasics& basics)
		{
			auto* engine = context.engine;
			auto mushroom = std::make_shared<Entity::MushroomNpc>(
				basics.name.empty() ? "Gzib" : basics.name,
				basics.position.x,
				basics.position.y,
				engine,
				data.value("follow_checkpoint", "Checkpoint Gziba"));
			mushroom->setAudioManager(&engine->getAudioManager());
			return mushroom;
		}

		std::shared_ptr<Entity::Entity> createVillageHeadNpc(
			const nlohmann::json& data,
			const SpawnContext& context,
			const EntityFactoryDetail::SpawnBasics& basics)
		{
			(void)data;
			auto* engine = context.engine;
			auto village_head = std::make_shared<Entity::VillageHeadNpc>(
				basics.name.empty() ? "Soltys" : basics.name,
				basics.position.x,
				basics.position.y,
				engine);
			village_head->setAudioManager(&engine->getAudioManager());
			return village_head;
		}

		std::shared_ptr<Entity::Entity> createSzeptuchaNpc(
			const nlohmann::json& data,
			const SpawnContext& context,
			const EntityFactoryDetail::SpawnBasics& basics)
		{
			(void)data;
			auto* engine = context.engine;
			auto szeptucha = std::make_shared<Entity::SzeptuchaNpc>(
				basics.name.empty() ? "Szeptucha" : basics.name,
				basics.position.x,
				basics.position.y,
				engine);
			szeptucha->setAudioManager(&engine->getAudioManager());
			return szeptucha;
		}

		std::shared_ptr<Entity::Entity> createWandaCorpseNpc(
			const nlohmann::json& data,
			const SpawnContext& context,
			const EntityFactoryDetail::SpawnBasics& basics)
		{
			(void)data;
			auto* engine = context.engine;
			auto corpse = std::make_shared<Entity::WandaCorpseNpc>(
				basics.name.empty() ? "Zwloki Wandy" : basics.name,
				basics.position.x,
				basics.position.y,
				engine);
			corpse->setAudioManager(&engine->getAudioManager());
			return corpse;
		}

		std::shared_ptr<Entity::Entity> createGenericStoryNpc(
			const nlohmann::json& data,
			const SpawnContext& context,
			const EntityFactoryDetail::SpawnBasics& basics)
		{
			auto* engine = context.engine;
			auto story_npc = std::make_shared<Entity::GenericStoryNpc>(
				basics.name.empty() ? "NPC" : basics.name,
				basics.position.x,
				basics.position.y,
				engine,
				data);
			story_npc->setAudioManager(&engine->getAudioManager());
			return story_npc;
		}

		std::shared_ptr<Entity::Entity> createForestLostGroupNpc(
			const nlohmann::json& data,
			const SpawnContext& context,
			const EntityFactoryDetail::SpawnBasics& basics)
		{
			auto* engine = context.engine;
			auto group = std::make_shared<Entity::ForestLostGroupNpc>(
				basics.name.empty() ? "Forest Lost NPC" : basics.name,
				basics.position.x,
				basics.position.y,
				engine,
				data);
			group->setAudioManager(&engine->getAudioManager());
			return group;
		}

		std::shared_ptr<Entity::Entity> createCemeterySurvivorGroupNpc(
			const nlohmann::json& data,
			const SpawnContext& context,
			const EntityFactoryDetail::SpawnBasics& basics)
		{
			auto* engine = context.engine;
			auto group = std::make_shared<Entity::CemeterySurvivorGroupNpc>(
				basics.name.empty() ? "Ocaleni z cmentarza" : basics.name,
				basics.position.x,
				basics.position.y,
				engine,
				data);
			group->setAudioManager(&engine->getAudioManager());
			return group;
		}

	}

	std::shared_ptr<Entity::Entity> EntityFactory::createNPC(
		const nlohmann::json& data, const SpawnContext& context)
	{
		using EntityFactoryDetail::NpcCreatorFn;

		static const std::unordered_map<std::string, NpcCreatorFn> npc_creators = {
			{"cat", &createCatNpc},
			{"mushroom", &createMushroomNpc},
			{"village_head", &createVillageHeadNpc},
			{"szeptucha", &createSzeptuchaNpc},
			{"wanda_corpse", &createWandaCorpseNpc},
			{"story_human", &createGenericStoryNpc},
			{"herbalist", &createGenericStoryNpc},
			{"forest_lost_group", &createForestLostGroupNpc},
			{"cemetery_survivor_group", &createCemeterySurvivorGroupNpc}
		};

		const std::string npc_class = data.value("npc_class", "cat");
		const auto basics = EntityFactoryDetail::readBasics(data, "NPC");

		if (const auto creator = npc_creators.find(npc_class); creator != npc_creators.end())
			return creator->second(data, context, basics);

		Core::Logger::errorLog("EntityFactory: nieznana klasa NPC: " + npc_class);
		return nullptr;
	}

} // namespace Nawia::World
