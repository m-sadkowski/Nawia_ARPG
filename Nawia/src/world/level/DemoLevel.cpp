#include "DemoLevel.h"

#include <Devil.h>
#include <Bandit.h>
#include <WalkingDead.h>
#include <Checkpoint.h>
#include <Chest.h>
#include <Cat.h>
#include <StaticObject.h>
#include <KnifeThrowAbility.h>

#include <Engine.h>
#include <Logger.h>
#include <MathUtils.h>

namespace Nawia::World {

	DemoLevel::DemoLevel() : _map(nullptr) {}

	DemoLevel::~DemoLevel() {}

	void DemoLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu DemoLevel...");

		// initialize map object
		_map = std::make_unique<Core::Map>(engine->getResourceManager());
		_map->loadMap("demo_map/demo.glb", 2.0f);

		auto& rm = engine->getResourceManager();
		auto& em = engine->getEntityManager();
		auto& itemDB = engine->getItemDatabase();
		auto& loottable = engine->getLoottable();

		em.clearNonPlayerEntities();

		const auto player = engine->getPlayer();
		if (player) {
			player->respawn(); // reset HP, flags, animation
			player->setX(-4.3f);
			player->setY(33.0f);
			player->setRespawnPoint({-4.3f, 33.0f});
			player->stop();
		}

		std::shared_ptr<Entity::Devil> devil = Entity::DevilBuilder()
			.setName("Devil")
			.setPosition({ 0.f, 12.1f })
			.setMap(_map.get())
			.setMaxHp(120)
			.setTarget(player)
			.build();
		em.addEntity(devil);

		std::shared_ptr<Entity::Bandit> bandit = Entity::BanditBuilder()
			.setName("Bandyta")
			.setPosition({ 15.9f, 12.6f })
			.setMap(_map.get())
			.setMaxHp(80)
			.setTarget(player)
			.build();
		bandit->addAbility(std::make_shared<Entity::KnifeThrowAbility>("../assets/models/knife.glb", 0.05f, nullptr, nullptr, 180.0f));
		em.addEntity(bandit);

		std::shared_ptr<Entity::WalkingDead> walking_dead = Entity::WalkingDeadBuilder()
			.setName("Walking Dead")
			.setPosition({ 23.36f, 20.49f })
			.setMap(_map.get())
			.setMaxHp(80)
			.setTarget(player)
			.build();
		em.addEntity(walking_dead);

		const auto tree_tex = rm.getTexture("../assets/textures/chest.png");
		std::shared_ptr<Entity::StaticObject> tree = Entity::StaticObjectBuilder()
			.setName("Drzewo")
			.setPosition({ 5.0f, 5.0f })
			.setMaxHp(9999)
			.setTexture(tree_tex)
			.build();
		em.addEntity(tree);

		const auto chest_tex = rm.getTexture("../assets/textures/chest.png");
		auto test_chest = std::make_shared<Entity::Chest>("Stara Skrzynia", -13.44f, -21.44f, chest_tex);
		test_chest->initializeInventory(loottable, Item::LOOTTABLE_TYPE::CHEST_NOOB);
		em.addEntity(test_chest);

		const auto david_chest = std::make_shared<Entity::Chest>("Skrzynia Davida", 5.6f, 8.57f, chest_tex);
		if (const auto fish = itemDB.createItem(6)) david_chest->addItem(fish);
		david_chest->setLocked(true, 5);
		em.addEntity(david_chest);

		auto cat = std::make_shared<Entity::Cat>("Kot Olga", 0.35f, -18.23f, chest_tex);
		engine->getDialogueManager().createCatDialogue(engine, cat.get());
		cat->initializeInventory(loottable, Item::LOOTTABLE_TYPE::CAT);
		em.addEntity(cat);

		auto test_checkpoint = std::make_shared<Entity::Checkpoint>("Punkt Kontrolny", 20.0f, 20.0f);
		em.addEntity(test_checkpoint);
	}

	void DemoLevel::onExit(Core::Engine* engine) {
		if (engine)
			engine->getEntityManager().clearNonPlayerEntities();
	}

	Core::Map* DemoLevel::getMap() const {
		return _map.get();
	}

	std::string DemoLevel::getName() const {
		return "DemoLevel";
	}

} // namespace Nawia::World
