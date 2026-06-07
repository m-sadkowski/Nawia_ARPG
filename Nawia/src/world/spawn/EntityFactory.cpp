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

using json = nlohmann::json;

namespace Nawia::World {

	namespace {

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

	}

	std::shared_ptr<Entity::Entity> EntityFactory::create(
		const std::string& type,
		const json& data,
		Core::Engine* engine,
		Core::Map* map)
	{
		if (type == "devil")         return createDevil(data, engine, map);
		if (type == "witch")         return createWitch(data, engine, map);
		if (type == "bandit")        return createBandit(data, engine, map);
		if (type == "walking_dead")  return createWalkingDead(data, engine, map);
		if (type == "frog")          return createFrog(data, engine, map);
		if (type == "worm")          return createWorm(data, engine, map);
		if (type == "spider")        return createSpider(data, engine, map);
		if (type == "mini_mushroom_infected") return createMiniMushroomInfected(data, engine, map);
		if (type == "friend")        return createFriend(data, engine, map);
		if (type == "chest")         return createChest(data, engine);
		if (type == "npc")           return createNPC(data, engine);
		if (type == "mini_mushroom_prop") return createMiniMushroomProp(data, engine);
		if (type == "static_object") return createStaticObject(data, engine);
		if (type == "checkpoint")    return createCheckpoint(data);
		if (type == "checkpoint_mushroom_npc") return createMushroomWaypoint(data);
		if (type == "story_anchor")  return createStoryAnchor(data);
		if (type == "herbalist_hub") return createHerbalistHub(data);
		if (type == "teleport")      return createTeleport(data, engine);
		if (type == "boss_trigger")  return createBossTrigger(data, engine);
		if (type == "story_trigger") return createStoryTrigger(data, engine);

		Core::Logger::errorLog("EntityFactory: nieznany typ encji: " + type);
		return nullptr;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createDevil(
		const json& data, Core::Engine* engine, Core::Map* map)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const int hp = data.value("hp", 100);
		const std::string name = data.value("name", "Devil");

		auto player = engine->getPlayer();

		auto devil = Entity::DevilBuilder()
			.setName(name)
			.setPosition({x, y})
			.setMap(map)
			.setMaxHp(hp)
			.setTarget(player)
			.setAudioManager(&engine->getAudioManager())
			.build();

		return devil;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createWitch(
		const json& data, Core::Engine* engine, Core::Map* map)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const int hp = data.value("hp", 160);
		const std::string name = data.value("name", "Czarownica");

		auto witch = Entity::WitchBuilder()
			.setName(name)
			.setPosition({x, y})
			.setMap(map)
			.setMaxHp(hp)
			.setTarget(engine ? engine->getPlayer() : nullptr)
			.setAudioManager(engine ? &engine->getAudioManager() : nullptr)
			.build();

		return witch;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createBandit(
		const json& data, Core::Engine* engine, Core::Map* map)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const int hp = data.value("hp", 80);
		const std::string name = data.value("name", "Bandyta");

		auto player = engine->getPlayer();

		auto bandit = Entity::BanditBuilder()
			.setName(name)
			.setPosition({x, y})
			.setMap(map)
			.setMaxHp(hp)
			.setTarget(player)
			.setAudioManager(&engine->getAudioManager())
			.build();

		bandit->ensureKnifeThrowAbility(&engine->getResourceManager());

		return bandit;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createWalkingDead(
		const json& data, Core::Engine* engine, Core::Map* map)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const int hp = data.value("hp", 80);
		const std::string name = data.value("name", "Walking Dead");

		auto player = engine->getPlayer();

		auto wd = Entity::WalkingDeadBuilder()
			.setName(name)
			.setPosition({x, y})
			.setMap(map)
			.setMaxHp(hp)
			.setTarget(player)
			.setAudioManager(&engine->getAudioManager())
			.build();

		return wd;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createFrog(
		const json& data, Core::Engine* engine, Core::Map* map)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const int hp = data.value("hp", 95);
		const std::string name = data.value("name", "Ropuch");

		auto frog = std::shared_ptr<Entity::Frog>(Entity::FrogBuilder()
			.setName(name)
			.setPosition({x, y})
			.setMap(map)
			.setEngine(engine)
			.setMaxHp(hp)
			.setTarget(engine->getPlayer())
			.setAudioManager(&engine->getAudioManager())
			.build());
		return frog;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createWorm(
		const json& data, Core::Engine* engine, Core::Map* map)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const int hp = data.value("hp", 35);
		const std::string name = data.value("name", "Robal");

		auto worm = std::shared_ptr<Entity::Worm>(Entity::WormBuilder()
			.setName(name)
			.setPosition({x, y})
			.setMap(map)
			.setMaxHp(hp)
			.setTarget(engine->getPlayer())
			.setAudioManager(&engine->getAudioManager())
			.build());
		return worm;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createSpider(
		const json& data, Core::Engine* engine, Core::Map* map)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const int hp = data.value("hp", 240);
		const std::string name = data.value("name", "Pajak");

		auto spider = std::shared_ptr<Entity::Spider>(Entity::SpiderBuilder()
			.setName(name)
			.setPosition({x, y})
			.setMap(map)
			.setMaxHp(hp)
			.setTarget(engine ? engine->getPlayer() : nullptr)
			.setAudioManager(engine ? &engine->getAudioManager() : nullptr)
			.build());
		return spider;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createMiniMushroomInfected(
		const json& data, Core::Engine* engine, Core::Map* map)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const int hp = data.value("hp", 45);
		const std::string name = data.value("name", "Zly Gzibek");

		auto mushroom = std::shared_ptr<Entity::MiniMushroomInfected>(Entity::MiniMushroomInfectedBuilder()
			.setName(name)
			.setPosition({x, y})
			.setMap(map)
			.setMaxHp(hp)
			.setTarget(engine->getPlayer())
			.setAudioManager(&engine->getAudioManager())
			.build());
		return mushroom;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createFriend(
		const json& data, Core::Engine* engine, Core::Map* map)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const int hp = data.value("hp", 100);
		const std::string name = data.value("name", "Friend");

		auto friend_entity = Entity::FriendBuilder()
			.setName(name)
			.setPosition({x, y})
			.setMap(map)
			.setMaxHp(hp)
			.setAudioManager(&engine->getAudioManager())
			.build();

		auto& rm = engine->getResourceManager();
		const auto sword_slash_icon = rm.getTexture("assets/textures/icons/sword_slash_icon.png");
		friend_entity->addAbility(std::make_shared<Entity::SwordSlashAbility>(nullptr, sword_slash_icon));

		return friend_entity;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createChest(
		const json& data, Core::Engine* engine)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const std::string name = data.value("name", "Skrzynia");

		auto chest = std::make_shared<Entity::Chest>(name, x, y, nullptr);
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
		const json& data, Core::Engine* engine)
	{
		const std::string npc_class = data.value("npc_class", "cat");
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const std::string name = data.value("name", "NPC");

		if (npc_class == "cat") {
			auto cat = std::make_shared<Entity::Cat>(name, x, y, nullptr);
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

		if (npc_class == "mushroom") {
			auto mushroom = std::make_shared<Entity::MushroomNpc>(
				name.empty() ? "Gzib" : name,
				x,
				y,
				engine,
				data.value("follow_checkpoint", "Checkpoint Gziba"));
			mushroom->setAudioManager(&engine->getAudioManager());
			return mushroom;
		}

		if (npc_class == "village_head") {
			auto village_head = std::make_shared<Entity::VillageHeadNpc>(name.empty() ? "Soltys" : name, x, y, engine);
			village_head->setAudioManager(&engine->getAudioManager());
			return village_head;
		}

		if (npc_class == "szeptucha") {
			auto szeptucha = std::make_shared<Entity::SzeptuchaNpc>(name.empty() ? "Szeptucha" : name, x, y, engine);
			szeptucha->setAudioManager(&engine->getAudioManager());
			return szeptucha;
		}

		if (npc_class == "wanda_corpse") {
			auto corpse = std::make_shared<Entity::WandaCorpseNpc>(name.empty() ? "Zwloki Wandy" : name, x, y, engine);
			corpse->setAudioManager(&engine->getAudioManager());
			return corpse;
		}

		if (npc_class == "story_human" || npc_class == "herbalist") {
			auto story_npc = std::make_shared<Entity::GenericStoryNpc>(name.empty() ? "NPC" : name, x, y, engine, data);
			story_npc->setAudioManager(&engine->getAudioManager());
			return story_npc;
		}

		if (npc_class == "forest_lost_group") {
			auto group = std::make_shared<Entity::ForestLostGroupNpc>(name.empty() ? "Forest Lost NPC" : name, x, y, engine, data);
			group->setAudioManager(&engine->getAudioManager());
			return group;
		}

		if (npc_class == "cemetery_survivor_group") {
			auto group = std::make_shared<Entity::CemeterySurvivorGroupNpc>(name.empty() ? "Ocaleni z cmentarza" : name, x, y, engine, data);
			group->setAudioManager(&engine->getAudioManager());
			return group;
		}

		Core::Logger::errorLog("EntityFactory: nieznana klasa NPC: " + npc_class);
		return nullptr;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createMiniMushroomProp(
		const json& data, Core::Engine* engine)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const std::string name = data.value("name", "Gzibek");

		auto prop = std::make_shared<Entity::MiniMushroomProp>();
		prop->setName(name);
		prop->setX(x);
		prop->setY(y);
		prop->setAudioManager(&engine->getAudioManager());
		return prop;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createStaticObject(
		const json& data, Core::Engine* engine)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const int hp = data.value("hp", 9999);
		const std::string name = data.value("name", "Obiekt");
		const std::string model_path = resolveModelPath(readStringAlias(data, {"model", "model_path", "texture"}));

		auto object = Entity::StaticObjectBuilder()
			.setName(name)
			.setPosition({x, y})
			.setMaxHp(hp)
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

	std::shared_ptr<Entity::Entity> EntityFactory::createCheckpoint(const json& data)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const std::string name = data.value("name", "Punkt Kontrolny");

		return std::make_shared<Entity::Checkpoint>(name, x, y);
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createMushroomWaypoint(const json& data)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const std::string name = data.value("name", "Checkpoint Gziba");

		return Entity::StaticObjectBuilder()
			.setName(name)
			.setPosition({x, y})
			.setMaxHp(1)
			.build();
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createStoryAnchor(const json& data)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const std::string name = data.value("name", "Story Anchor");

		auto anchor = std::make_shared<Entity::Entity>(name, x, y, nullptr, 1);
		anchor->setType(Entity::EntityType::NPCStatic);
		anchor->setFaction(Entity::Faction::None);
		return anchor;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createHerbalistHub(const json& data)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const std::string name = data.value("name", "Herbalist Hub");
		const float radius = data.value("radius", data.value("spawn_radius", 5.0f));

		return std::make_shared<Entity::HerbalistHub>(name, x, y, radius);
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createTeleport(
		const json& data, Core::Engine* engine)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const std::string name = data.value("name", "Teleport");
		const std::string target_location = data.value("target_location", "");

		if (target_location.empty()) {
			Core::Logger::errorLog("EntityFactory: Teleport wymaga pola 'target_location'");
		}

		auto teleport = std::make_shared<Entity::Teleport>(name, x, y, engine, target_location);
		teleport->setScale(data.value("scale", 1.0f));
		if (data.contains("rotation") && data["rotation"].is_number())
			teleport->setRotation(data["rotation"].get<float>());
		return teleport;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createBossTrigger(const json& data, Core::Engine* engine)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const float width = data.value("width", 4.0f);
		const float height = data.value("height", 4.0f);
		const std::string boss_id = data.value("boss_id", "");

		if (boss_id.empty()) {
			Core::Logger::errorLog("EntityFactory: boss_trigger wymaga pola 'boss_id'");
		} else if (engine) {
			engine->getBossManager().preloadBossFight(boss_id, engine);
		}

		return std::make_shared<Entity::BossArenaTrigger>(boss_id, x, y, width, height);
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createStoryTrigger(const json& data, Core::Engine* engine)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const float width = data.value("width", 4.0f);
		const float height = data.value("height", 4.0f);
		const std::string name = data.value("name", "Story Trigger");

		return std::make_shared<Entity::StoryTrigger>(name, x, y, width, height, engine, data);
	}

} // namespace Nawia::World
