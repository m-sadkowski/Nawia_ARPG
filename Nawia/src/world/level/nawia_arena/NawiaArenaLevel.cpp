#include "NawiaArenaLevel.h"

#include <Engine.h>
#include <FireballAbility.h>
#include <Logger.h>
#include <Player.h>
#include <ResourceManager.h>

#include <memory>

namespace Nawia::World {

	namespace {
		constexpr const char* ARENA_LIGHTING_FILE = "assets/maps/przedsionek_nawii_bright_lighting.json";
		constexpr const char* ARENA_FIREBALL_MODEL = "assets/models/fireball.glb";
		constexpr const char* ARENA_FIREBALL_ICON = "assets/textures/icons/fireball_icon.png";
		constexpr int ARENA_EQUIPMENT_ITEM_IDS[] = {1, 2, 3, 8, 9};
		constexpr int ARENA_BACKPACK_ITEM_IDS[] = {4, 5, 6, 7, 10, 11, 12};

		const std::vector<LevelLocationFile> ARENA_LOCATIONS = {
			{"Przedsionek Nawii", "assets/data/locations/przedsionek_nawii_siewca_arena.json"},
		};
	}

	std::vector<LevelLocationFile> NawiaArenaLevel::getLocationFiles() const {
		return ARENA_LOCATIONS;
	}

	void NawiaArenaLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu Przedsionek Nawii...");

		if (!engine)
			return;

		engine->getLightingSystem().loadLightingFromJson(ARENA_LIGHTING_FILE);
		activatePreparedLocations(engine);
		engine->getAudioManager().stopMusic();
	}

	void NawiaArenaLevel::onNewGameStarted(Core::Engine* engine) {
		if (!engine || !engine->getPlayer())
			return;

		auto player = engine->getPlayer();
		auto& item_database = engine->getItemDatabase();

		for (const int item_id : ARENA_EQUIPMENT_ITEM_IDS) {
			if (const auto item = item_database.createItem(item_id))
				player->equipItem(item);
		}

		for (const int item_id : ARENA_BACKPACK_ITEM_IDS) {
			if (const auto item = item_database.createItem(item_id))
				player->getBackpack().addItem(item);
		}

		const auto fireball_icon = engine->getResourceManager().getTexture(ARENA_FIREBALL_ICON);
		player->addAbility(std::make_shared<Entity::FireballAbility>(
			ARENA_FIREBALL_MODEL,
			0.5f,
			nullptr,
			fireball_icon,
			&engine->getResourceManager()));
	}

} // namespace Nawia::World
