#include "EntityFactory.h"
#include "EntityFactoryCommon.h"

#include <BossArenaTrigger.h>
#include <BossManager.h>
#include <Checkpoint.h>
#include <Chest.h>
#include <Engine.h>
#include <Entity.h>
#include <HerbalistHub.h>
#include <Logger.h>
#include <MiniMushroomProp.h>
#include <StaticObject.h>
#include <StoryTrigger.h>
#include <Teleport.h>

namespace Nawia::World {

	std::shared_ptr<Entity::Entity> EntityFactory::createChest(
		const nlohmann::json& data, const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Skrzynia");
		auto* engine = context.engine;

		auto chest = std::make_shared<Entity::Chest>(basics.name, basics.position.x, basics.position.y, nullptr);
		chest->setAudioManager(&engine->getAudioManager());

		if (data.contains("loottable")) {
			const std::string loottable_name = data["loottable"].get<std::string>();
			auto& loottable = engine->getLoottable();

			chest->initializeInventory(
				loottable,
				EntityFactoryDetail::parseLoottableType(loottable_name, Item::LOOTTABLE_TYPE::CHEST_NOOB)
			);
		}

		EntityFactoryDetail::addItemsFromJson(data, engine, [&](const std::shared_ptr<Item::Item>& item) {
			chest->addItem(item);
		});

		if (data.value("locked", false)) {
			const int key_id = data.value("key_id", -1);
			chest->setLocked(true, key_id);
		}

		return chest;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createMiniMushroomProp(
		const nlohmann::json& data, const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Gzibek");
		auto* engine = context.engine;

		auto prop = std::make_shared<Entity::MiniMushroomProp>();
		prop->setName(basics.name);
		prop->setX(basics.position.x);
		prop->setY(basics.position.y);
		prop->setAudioManager(&engine->getAudioManager());
		return prop;
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createStaticObject(
		const nlohmann::json& data, const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Obiekt", 9999);
		auto* engine = context.engine;
		const std::string model_path = Core::AssetPathUtils::resolveModelPath(
			Core::JsonUtils::readStringAlias(data, {"model", "model_path", "texture"})
		);

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
		const nlohmann::json& data,
		const SpawnContext& context)
	{
		(void)context;
		const auto basics = EntityFactoryDetail::readBasics(data, "Punkt Kontrolny");

		return std::make_shared<Entity::Checkpoint>(basics.name, basics.position.x, basics.position.y);
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createMushroomWaypoint(
		const nlohmann::json& data,
		const SpawnContext& context)
	{
		(void)context;
		const auto basics = EntityFactoryDetail::readBasics(data, "Checkpoint Gziba");

		return Entity::StaticObjectBuilder()
			.setName(basics.name)
			.setPosition(basics.position)
			.setMaxHp(1)
			.build();
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createStoryAnchor(
		const nlohmann::json& data,
		const SpawnContext& context)
	{
		(void)context;
		const auto basics = EntityFactoryDetail::readBasics(data, "Story Anchor");

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
		const nlohmann::json& data,
		const SpawnContext& context)
	{
		(void)context;
		const auto basics = EntityFactoryDetail::readBasics(data, "Herbalist Hub");
		const float radius = data.value("radius", data.value("spawn_radius", 5.0f));

		return std::make_shared<Entity::HerbalistHub>(
			basics.name,
			basics.position.x,
			basics.position.y,
			radius);
	}

	std::shared_ptr<Entity::Entity> EntityFactory::createTeleport(
		const nlohmann::json& data,
		const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Teleport");
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
		const nlohmann::json& data,
		const SpawnContext& context)
	{
		auto* engine = context.engine;
		const auto basics = EntityFactoryDetail::readBasics(data, "Boss Trigger");
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
		const nlohmann::json& data,
		const SpawnContext& context)
	{
		const auto basics = EntityFactoryDetail::readBasics(data, "Story Trigger");
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
