#include "EntityFactory.h"

#include <Backpack.h>
#include <Engine.h>
#include <Item.h>
#include <Logger.h>
#include <Map.h>

#include <Bandit.h>
#include <Devil.h>
#include <Friend.h>
#include <Cat.h>
#include <Chest.h>
#include <Checkpoint.h>
#include <StaticObject.h>
#include <Teleport.h>
#include <WalkingDead.h>

#include <KnifeThrowAbility.h>
#include <SwordSlashAbility.h>

#include <ItemDatabase.h>
#include <Loottable.h>

#include <json.hpp>

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

	}

	std::shared_ptr<Entity::Entity> EntityFactory::create(
		const std::string& type,
		const json& data,
		Core::Engine* engine,
		Core::Map* map)
	{
		if (type == "devil")         return createDevil(data, engine, map);
		if (type == "bandit")        return createBandit(data, engine, map);
		if (type == "walking_dead")  return createWalkingDead(data, engine, map);
		if (type == "friend")        return createFriend(data, engine, map);
		if (type == "chest")         return createChest(data, engine);
		if (type == "npc")           return createNPC(data, engine);
		if (type == "static_object") return createStaticObject(data, engine);
		if (type == "checkpoint")    return createCheckpoint(data);
		if (type == "teleport")      return createTeleport(data, engine);
		if (type == "boss_trigger")  return createBossTrigger(data);

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
			.build();

		return devil;
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
			.build();

		if (data.contains("abilities")) {
			for (const auto& ability_name : data["abilities"]) {
				const std::string ability_id = ability_name.get<std::string>();
				if (ability_id == "KnifeThrow") {
					bandit->addAbility(std::make_shared<Entity::KnifeThrowAbility>(
						"assets/models/knife.glb", 0.05f, nullptr, nullptr, 180.0f));
				}
			}
		}

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
			.build();

		return wd;
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

		auto& rm = engine->getResourceManager();
		const auto texture = rm.getTexture("assets/textures/chest.png");

		auto chest = std::make_shared<Entity::Chest>(name, x, y, texture);

		if (data.contains("loottable")) {
			const std::string loottable_name = data["loottable"].get<std::string>();
			auto& loottable = engine->getLoottable();

			chest->initializeInventory(
				loottable,
				parseLoottableType(loottable_name, Item::LOOTTABLE_TYPE::CHEST_NOOB)
			);
		}

		if (data.contains("items")) {
			auto& item_database = engine->getItemDatabase();
			for (const auto& item_id : data["items"]) {
				if (auto item = item_database.createItem(item_id.get<int>())) {
					chest->addItem(item);
				}
			}
		}

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

		auto& rm = engine->getResourceManager();
		const auto texture = rm.getTexture("assets/textures/chest.png");

		if (npc_class == "cat") {
			auto cat = std::make_shared<Entity::Cat>(name, x, y, texture);

			engine->getDialogueManager().createCatDialogue(engine, cat.get());

			if (data.contains("loottable")) {
				const std::string loottable_name = data["loottable"].get<std::string>();
				auto& loottable = engine->getLoottable();

				cat->initializeInventory(
					loottable,
					parseLoottableType(loottable_name, Item::LOOTTABLE_TYPE::CAT)
				);
			}

			constexpr int cat_key_id = 5;
			if (cat->getInventory() && !cat->getInventory()->getItem(0)) {
				if (const auto key = engine->getItemDatabase().createItem(cat_key_id))
					cat->addItem(key);
				else
					cat->addItem(std::make_shared<Item::Item>(
						cat_key_id,
						"Klucz Kota",
						"Klucz nalezacy do Kota Olgi.",
						Item::EquipmentSlot::None,
						nullptr));
			}

			return cat;
		}

		Core::Logger::errorLog("EntityFactory: nieznana klasa NPC: " + npc_class);
		return nullptr;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createStaticObject(
		const json& data, Core::Engine* engine)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const int hp = data.value("hp", 9999);
		const std::string name = data.value("name", "Obiekt");
		const std::string texture_path = data.value("texture", "assets/textures/chest.png");

		auto& rm = engine->getResourceManager();
		const auto texture = rm.getTexture(texture_path);

		auto object = Entity::StaticObjectBuilder()
			.setName(name)
			.setPosition({x, y})
			.setMaxHp(hp)
			.setTexture(texture)
			.build();

		return object;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createCheckpoint(const json& data)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const std::string name = data.value("name", "Punkt Kontrolny");

		return std::make_shared<Entity::Checkpoint>(name, x, y);
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

		return std::make_shared<Entity::Teleport>(name, x, y, engine, target_location);
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createBossTrigger(const json& data)
	{
		const float x = data.value("x", 0.0f);
		const float y = data.value("y", 0.0f);
		const float width = data.value("width", 5.0f);
		const float height = data.value("height", 5.0f);
		const std::string boss_id = data.value("boss_id", "");

		return std::make_shared<Entity::BossArenaTrigger>(boss_id, x, y, width, height);
	}

} // namespace Nawia::World
