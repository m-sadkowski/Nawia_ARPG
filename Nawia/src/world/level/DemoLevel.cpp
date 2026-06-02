#include "DemoLevel.h"

#include <Engine.h>
#include <FireballAbility.h>
#include <Logger.h>
#include <Player.h>
#include <ResourceManager.h>

#include <memory>

namespace Nawia::World {

	namespace {
		constexpr const char* DIABELSKI_LAS_MUSIC =
			"assets/audio/music/soundsbyamelia-baba-yaga-ritual-slavic-horror-with-hurdy-gurdy-amp-choir-422979.mp3";
		constexpr const char* LESNA_DOLINA_MUSIC =
			"assets/audio/music/soundsbyamelia-slavic-war-dance-drums-stomps-amp-war-pipes-422949.mp3";
		constexpr const char* DEMO_LIGHTING_FILE = "assets/maps/forest_lighting.json";
		constexpr const char* DEMO_FIREBALL_MODEL = "assets/models/fireball.glb";
		constexpr const char* DEMO_FIREBALL_ICON = "assets/textures/icons/fireball_icon.png";
		constexpr float LOCATION_MUSIC_VOLUME = 0.75f;
		constexpr int DEMO_EQUIPMENT_ITEM_IDS[] = {1, 2, 3, 8, 9};
		constexpr int DEMO_BACKPACK_ITEM_IDS[] = {4, 5, 6, 7, 10, 11, 12};

		const std::vector<LevelLocationFile> DEMO_LOCATIONS = {
			{"Diabelski Las", "assets/data/locations/diabelski_las.json"},
			{"Lesna Dolina", "assets/data/locations/lesna_dolina.json"},
		};
	}

	std::vector<LevelLocationFile> DemoLevel::getLocationFiles() const {
		return DEMO_LOCATIONS;
	}

	void DemoLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu Demo...");

		if (engine) {
			engine->getLightingSystem().loadLightingFromJson(DEMO_LIGHTING_FILE);
			activatePreparedLocations(engine);
			playLocationMusic(engine, getCurrentLocationName());
		}
	}

	void DemoLevel::onNewGameStarted(Core::Engine* engine) {
		if (!engine || !engine->getPlayer())
			return;

		auto player = engine->getPlayer();
		auto& item_database = engine->getItemDatabase();

		for (const int item_id : DEMO_EQUIPMENT_ITEM_IDS) {
			if (const auto item = item_database.createItem(item_id))
				player->equipItem(item);
		}

		for (const int item_id : DEMO_BACKPACK_ITEM_IDS) {
			if (const auto item = item_database.createItem(item_id))
				player->getBackpack().addItem(item);
		}

		const auto fireball_icon = engine->getResourceManager().getTexture(DEMO_FIREBALL_ICON);
		player->addAbility(std::make_shared<Entity::FireballAbility>(
			DEMO_FIREBALL_MODEL,
			0.5f,
			nullptr,
			fireball_icon,
			&engine->getResourceManager()));
	}

	void DemoLevel::changeLocation(Core::Engine* engine, const std::string& location_name) {
		Level::changeLocation(engine, location_name);
		playLocationMusic(engine, location_name);
	}

	void DemoLevel::playLocationMusic(Core::Engine* engine, const std::string& location_name) const {
		if (!engine)
			return;

		if (location_name == "Lesna Dolina") {
			engine->getAudioManager().playMusic(LESNA_DOLINA_MUSIC, true, LOCATION_MUSIC_VOLUME);
		} else {
			engine->getAudioManager().playMusic(DIABELSKI_LAS_MUSIC, true, LOCATION_MUSIC_VOLUME);
		}
	}

} // namespace Nawia::World
