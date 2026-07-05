#include "EntityFactory.h"

#include <BossManager.h>
#include <Engine.h>
#include <Entity.h>
#include <Item.h>
#include <Logger.h>
#include <Map.h>

#include <Bandit.h>
#include <Devil.h>
#include <Friend.h>
#include <BossArenaTrigger.h>
#include <Cat.h>
#include <CemeterySurvivorGroupNpc.h>
#include <Chest.h>
#include <Checkpoint.h>
#include <Frog.h>
#include <ForestLostGroupNpc.h>
#include <GenericStoryNpc.h>
#include <HerbalistHub.h>
#include <MiniMushroomInfected.h>
#include <MiniMushroomProp.h>
#include <MushroomNpc.h>
#include <RiftBinder.h>
#include <SzeptuchaNpc.h>
#include <spider/Spider.h>
#include <StaticObject.h>
#include <StoryTrigger.h>
#include <Teleport.h>
#include <VillageHeadNpc.h>
#include <WalkingDead.h>
#include <WandaCorpseNpc.h>
#include <witch/Witch.h>
#include <Worm.h>

#include <SwordSlashAbility.h>

#include <ItemDatabase.h>
#include <Loottable.h>

#include <json.hpp>

#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <unordered_map>

using json = nlohmann::json;

namespace Nawia::World {

	namespace {

		struct SpawnBasics {
			Vector2 position = {0.0f, 0.0f};
			int hp = 1;
			std::string name;
		};

		SpawnBasics readBasics(const json& data, const std::string& default_name, const int default_hp = 1) {
			return {
				{data.value("x", 0.0f), data.value("y", 0.0f)},
				data.value("hp", default_hp),
				data.value("name", default_name)
			};
		}

		Item::LOOTTABLE_TYPE parseLoottableType(
			const std::string& loottable_name,
			Item::LOOTTABLE_TYPE default_type
		) {
			if (loottable_name == "CHEST_BAD") return Item::LOOTTABLE_TYPE::CHEST_BAD;
			if (loottable_name == "CAT") return Item::LOOTTABLE_TYPE::CAT;
			if (loottable_name == "CHEST_NOOB") return Item::LOOTTABLE_TYPE::CHEST_NOOB;
			return default_type;
		}

		std::string readStringAlias(const json& data, const std::initializer_list<const char*> keys) {
			for (const char* key : keys) {
				if (data.contains(key) && data[key].is_string())
					return data[key].get<std::string>();
			}

			return "";
		}

		std::string resolveModelPath(std::string model_path) {
			std::ranges::replace(model_path, '\\', '/');
			if (model_path.empty() || model_path.rfind("assets/", 0) == 0)
				return model_path;

			if (std::filesystem::path(model_path).has_parent_path())
				return model_path;

			return "assets/models/" + model_path;
		}

		template <typename AddItem>
		void addItemsFromJson(const json& data, Core::Engine* engine, AddItem add_item) {
			if (!engine || !data.contains("items") || !data["items"].is_array())
				return;

			auto& item_database = engine->getItemDatabase();
			for (const auto& item_id : data["items"]) {
				if (!item_id.is_number_integer())
					continue;

				if (auto item = item_database.createItem(item_id.get<int>()))
					add_item(item);
			}
		}

		using CreatorFn = std::shared_ptr<Entity::Entity> (*)(const json&, const SpawnContext&);
		using NpcCreatorFn = std::shared_ptr<Entity::Entity> (*)(const json&, const SpawnContext&, const SpawnBasics&);

		std::shared_ptr<Entity::Entity> createCatNpc(
			const json& data,
			const SpawnContext& context,
			const SpawnBasics& basics)
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
					parseLoottableType(loottable_name, Item::LOOTTABLE_TYPE::CAT)
				);
			}

			addItemsFromJson(data, engine, [&](const std::shared_ptr<Item::Item>& item) {
				cat->addItem(item);
			});
			if (auto key = engine->getItemDatabase().createItem(5))
				cat->addItem(key);

			return cat;
		}

		std::shared_ptr<Entity::Entity> createMushroomNpc(
			const json& data,
			const SpawnContext& context,
			const SpawnBasics& basics)
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
			const json& data,
			const SpawnContext& context,
			const SpawnBasics& basics)
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
			const json& data,
			const SpawnContext& context,
			const SpawnBasics& basics)
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
			const json& data,
			const SpawnContext& context,
			const SpawnBasics& basics)
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
			const json& data,
			const SpawnContext& context,
			const SpawnBasics& basics)
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
			const json& data,
			const SpawnContext& context,
			const SpawnBasics& basics)
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
			const json& data,
			const SpawnContext& context,
			const SpawnBasics& basics)
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

	std::shared_ptr<Entity::Entity> EntityFactory::create(
		const std::string& type,
		const json& data,
		Core::Engine* engine,
		Core::Map* map)
	{
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

	std::shared_ptr<Entity::Entity> EntityFactory::createDevil(
		const json& data, const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Devil", 100);
		auto* engine = context.engine;
		auto player = engine->getPlayer();

		auto devil = Entity::DevilBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(player)
			.setAudioManager(&engine->getAudioManager())
			.build();

		return devil;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createRiftBinder(
		const json& data, const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Siewca Chaosu", 420);
		auto* engine = context.engine;

		auto boss = Entity::RiftBinderBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(engine ? engine->getPlayer() : nullptr)
			.setAudioManager(engine ? &engine->getAudioManager() : nullptr)
			.build();

		return boss;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createWitch(
		const json& data, const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Czarownica", 160);
		auto* engine = context.engine;

		auto witch = Entity::WitchBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(engine ? engine->getPlayer() : nullptr)
			.setAudioManager(engine ? &engine->getAudioManager() : nullptr)
			.build();

		return witch;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createBandit(
		const json& data, const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Bandyta", 80);
		auto* engine = context.engine;
		auto player = engine->getPlayer();

		auto bandit = Entity::BanditBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(player)
			.setAudioManager(&engine->getAudioManager())
			.build();

		bandit->ensureKnifeThrowAbility(&engine->getResourceManager());

		return bandit;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createWalkingDead(
		const json& data, const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Walking Dead", 80);
		auto* engine = context.engine;
		auto player = engine->getPlayer();

		auto wd = Entity::WalkingDeadBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(player)
			.setAudioManager(&engine->getAudioManager())
			.build();

		return wd;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createFrog(
		const json& data, const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Ropuch", 95);
		auto* engine = context.engine;

		auto frog = std::shared_ptr<Entity::Frog>(Entity::FrogBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setEngine(engine)
			.setMaxHp(basics.hp)
			.setTarget(engine->getPlayer())
			.setAudioManager(&engine->getAudioManager())
			.build());
		return frog;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createWorm(
		const json& data, const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Robal", 35);
		auto* engine = context.engine;

		auto worm = std::shared_ptr<Entity::Worm>(Entity::WormBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(engine->getPlayer())
			.setAudioManager(&engine->getAudioManager())
			.build());
		return worm;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createSpider(
		const json& data, const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Pajak", 240);
		auto* engine = context.engine;

		auto spider = std::shared_ptr<Entity::Spider>(Entity::SpiderBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(engine ? engine->getPlayer() : nullptr)
			.setAudioManager(engine ? &engine->getAudioManager() : nullptr)
			.build());
		return spider;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createMiniMushroomInfected(
		const json& data, const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Zly Gzibek", 45);
		auto* engine = context.engine;

		auto mushroom = std::shared_ptr<Entity::MiniMushroomInfected>(Entity::MiniMushroomInfectedBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setTarget(engine->getPlayer())
			.setAudioManager(&engine->getAudioManager())
			.build());
		return mushroom;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createFriend(
		const json& data, const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Friend", 100);
		auto* engine = context.engine;

		auto friend_entity = Entity::FriendBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMap(context.map)
			.setMaxHp(basics.hp)
			.setAudioManager(&engine->getAudioManager())
			.build();

		auto& rm = engine->getResourceManager();
		const auto sword_slash_icon = rm.getTexture("assets/textures/icons/sword_slash_icon.png");
		friend_entity->addAbility(std::make_shared<Entity::SwordSlashAbility>(nullptr, sword_slash_icon));

		return friend_entity;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createChest(
		const json& data, const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Skrzynia");
		auto* engine = context.engine;

		auto chest = std::make_shared<Entity::Chest>(basics.name, basics.position.x, basics.position.y, nullptr);
		chest->setAudioManager(&engine->getAudioManager());

		if (data.contains("loottable")) {
			const std::string loottable_name = data["loottable"].get<std::string>();
			auto& loottable = engine->getLoottable();

			chest->initializeInventory(
				loottable,
				parseLoottableType(loottable_name, Item::LOOTTABLE_TYPE::CHEST_NOOB)
			);
		}

		addItemsFromJson(data, engine, [&](const std::shared_ptr<Item::Item>& item) {
			chest->addItem(item);
		});

		if (data.value("locked", false)) {
			const int key_id = data.value("key_id", -1);
			chest->setLocked(true, key_id);
		}

		return chest;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createNPC(
		const json& data, const SpawnContext& context)
	{
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
		const auto basics = readBasics(data, "NPC");

		if (const auto creator = npc_creators.find(npc_class); creator != npc_creators.end())
			return creator->second(data, context, basics);

		Core::Logger::errorLog("EntityFactory: nieznana klasa NPC: " + npc_class);
		return nullptr;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createMiniMushroomProp(
		const json& data, const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Gzibek");
		auto* engine = context.engine;

		auto prop = std::make_shared<Entity::MiniMushroomProp>();
		prop->setName(basics.name);
		prop->setX(basics.position.x);
		prop->setY(basics.position.y);
		prop->setAudioManager(&engine->getAudioManager());
		return prop;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createStaticObject(
		const json& data, const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Obiekt", 9999);
		auto* engine = context.engine;
		const std::string model_path = resolveModelPath(readStringAlias(data, {"model", "model_path", "texture"}));

		auto object = Entity::StaticObjectBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMaxHp(basics.hp)
			.setAudioManager(&engine->getAudioManager())
			.build();

		if (!model_path.empty()) {
			object->loadModel(model_path);
			object->setScale(data.value("scale", 1.0f));

			if (data.contains("rotation") && data["rotation"].is_number())
				object->setRotation(data["rotation"].get<float>());
		} else {
			Core::Logger::errorLog("EntityFactory: static_object wymaga pola 'model'");
		}

		return object;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createCheckpoint(
		const json& data,
		const SpawnContext& context)
	{
		(void)context;
		const auto basics = readBasics(data, "Punkt Kontrolny");

		return std::make_shared<Entity::Checkpoint>(basics.name, basics.position.x, basics.position.y);
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createMushroomWaypoint(
		const json& data,
		const SpawnContext& context)
	{
		(void)context;
		const auto basics = readBasics(data, "Checkpoint Gziba");

		return Entity::StaticObjectBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMaxHp(1)
			.build();
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createStoryAnchor(
		const json& data,
		const SpawnContext& context)
	{
		(void)context;
		const auto basics = readBasics(data, "Story Anchor");

		auto anchor = std::make_shared<Entity::Entity>(
			basics.name,
			basics.position.x,
			basics.position.y,
			nullptr,
			1);
		anchor->setType(Entity::EntityType::NPCStatic);
		anchor->setFaction(Entity::Faction::None);
		return anchor;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createHerbalistHub(
		const json& data,
		const SpawnContext& context)
	{
		(void)context;
		const auto basics = readBasics(data, "Herbalist Hub");
		const float radius = data.value("radius", data.value("spawn_radius", 5.0f));

		return std::make_shared<Entity::HerbalistHub>(
			basics.name,
			basics.position.x,
			basics.position.y,
			radius);
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createTeleport(
		const json& data,
		const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Teleport");
		const std::string target_location = data.value("target_location", "");

		if (target_location.empty()) {
			Core::Logger::errorLog("EntityFactory: Teleport wymaga pola 'target_location'");
		}

		auto teleport = std::make_shared<Entity::Teleport>(
			basics.name,
			basics.position.x,
			basics.position.y,
			context.engine,
			target_location);
		teleport->setScale(data.value("scale", 1.0f));
		if (data.contains("rotation") && data["rotation"].is_number())
			teleport->setRotation(data["rotation"].get<float>());
		return teleport;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createBossTrigger(
		const json& data,
		const SpawnContext& context)
	{
		auto* engine = context.engine;
		const auto basics = readBasics(data, "Boss Trigger");
		const float width = data.value("width", 4.0f);
		const float height = data.value("height", 4.0f);
		const std::string boss_id = data.value("boss_id", "");

		if (boss_id.empty()) {
			Core::Logger::errorLog("EntityFactory: boss_trigger wymaga pola 'boss_id'");
		} else if (engine) {
			engine->getBossManager().preloadBossFight(boss_id, engine);
		}

		return std::make_shared<Entity::BossArenaTrigger>(
			boss_id,
			basics.position.x,
			basics.position.y,
			width,
			height);
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createStoryTrigger(
		const json& data,
		const SpawnContext& context)
	{
		const auto basics = readBasics(data, "Story Trigger");
		const float width = data.value("width", 4.0f);
		const float height = data.value("height", 4.0f);

		return std::make_shared<Entity::StoryTrigger>(
			basics.name,
			basics.position.x,
			basics.position.y,
			width,
			height,
			context.engine,
			data);
	}

} // namespace Nawia::World
