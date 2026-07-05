#include "Engine.h"

#include <AssetPreloader.h>
#include <Level.h>
#include <Logger.h>
#include <PlayerAbilityFactory.h>
#include <PlayerController.h>

#include <utility>
#include <vector>

namespace Nawia::Core {

	namespace {

		constexpr Vector2 k_initial_player_spawn = {0.0f, 0.0f};
		constexpr const char* MENU_MUSIC_PATH = "assets/audio/music/soulfuljamtracks-slavic-folk-308126.mp3";

	}

	void Engine::processLoading() {
		if (_loading_kind == LoadingKind::None)
			return;

		const size_t total_assets = _loading_manifest.size();
		if (_loading_asset_index < total_assets) {
			const auto& entry = _loading_manifest.entries()[_loading_asset_index];
			AssetPreloader::loadManifestStep(_loading_manifest, _loading_asset_index, _resource_manager);
			_loading_asset_index++;
			_loading_progress = static_cast<float>(_loading_asset_index) / static_cast<float>(total_assets);
			_loading_status = entry.label;
			return;
		}

		if (_loading_kind == LoadingKind::Startup) {
			finishStartupLoading();
			return;
		}

		if (_loading_kind == LoadingKind::Level) {
			finishLevelLoading();
		}
	}

	void Engine::finishStartupLoading() {
		_loading_status = "Inicjalizacja systemow...";
		_loading_progress = 1.0f;

		_item_database.loadDatabase("assets/data/items.json", _resource_manager);
		Logger::debugLog("Zaladowano baze danych przedmiotow");

		_loottable.loadLootTables("assets/data/loottables.json", _item_database);
		_quest_manager.loadFromJson("assets/data/quests.json");
		_boss_manager.loadFromJson("assets/data/bosses.json");

		Entity::Entity::setSharedResourceManager(&_resource_manager);
		createFreshPlayer();

		_ui_handler = std::make_unique<UI::UIHandler>();
		_ui_handler->initialize(_player, _entity_manager.get(), _resource_manager, &_quest_manager, &_settings);
		_ui_handler->setDialogueAudioManager(&_audio_manager);
		_ui_handler->setLevelManager(_level_manager.get());
		_ui_handler->setSaveGameManager(&_save_game_manager);

		_custom_cursor.initialize(_resource_manager);

		_audio_manager.playMusic(MENU_MUSIC_PATH, true, 1.f);

		_loading_kind = LoadingKind::None;
		_game_state = GameState::Menu;
	}

	void Engine::queueLevelLoad(
		const std::string& level_name,
		const std::string& initial_location,
		const bool is_new_game,
		const int default_slot
	) {
		const std::string resolved_level = level_name.empty() ? "Demo" : level_name;
		_pending_level_name = resolved_level;
		_pending_initial_location = initial_location;
		_pending_is_new_game = is_new_game;
		_pending_new_game_slot = default_slot;

		_loading_kind = LoadingKind::Level;
		_loading_asset_index = 0;
		_loading_progress = 0.0f;
		_loading_title = "Ladowanie poziomu";
		_loading_status = "Przygotowywanie listy zasobow...";
		_game_state = GameState::Loading;

		const auto level = _level_manager->getRegisteredLevel(resolved_level);
		std::vector<World::LocationDefinition> definitions;
		if (level && !level->getLocationFiles().empty()) {
			const std::string start_location = initial_location.empty()
				? level->getDefaultInitialLocation()
				: initial_location;
			_loading_manifest = AssetLoadManifest::buildForLocationFiles(level->getLocationFiles(), definitions);
			level->setPreparedLocationDefinitions(std::move(definitions), start_location);
		} else {
			_loading_manifest = {};
		}
	}

	void Engine::finishLevelLoading() {
		_loading_status = "Budowanie swiata...";
		_loading_progress = 1.0f;

		Entity::Entity::setSharedResourceManager(&_resource_manager);
		_audio_manager.stopMusic();
		_ping_manager.clear();
		_agent_command_interface.clear();
		_level_manager->changeLevel(_pending_level_name, this);

		if (_has_pending_save) {
			_save_game_manager.applySaveState(*this, _pending_save_state, _pending_save_slot);
			_has_pending_save = false;
			_pending_save_state = {};
			_game_state = GameState::Playing;
			if (_ui_handler) {
				_ui_handler->onLevelLoaded();
				_ui_handler->showNotification("Gra wczytana.", 3.0f);
			}
		} else {
			_game_state = GameState::Playing;
			if (_ui_handler)
				_ui_handler->onLevelLoaded();

			if (_pending_is_new_game && _level_manager && _level_manager->getCurrentLevel())
				_level_manager->getCurrentLevel()->onNewGameStarted(this);

			if (_pending_is_new_game && _pending_new_game_slot > 0)
				saveCurrentGame(_pending_new_game_slot);
		}

		_loading_kind = LoadingKind::None;
		_pending_is_new_game = false;
		_pending_new_game_slot = 0;
	}

	void Engine::createFreshPlayer() {
		_player = Entity::PlayerBuilder(this).setPosition(k_initial_player_spawn).build();
		_player->setAudioManager(&_audio_manager);

		const auto& player_setup = Entity::PlayerAbilityFactory::getPlayerSetupConfig();
		for (const auto& ability : Entity::PlayerAbilityFactory::createUnarmedAbilities(player_setup, _resource_manager))
			_player->addAbility(ability);

		_controller = std::make_unique<PlayerController>(this, _player);

		if (_entity_manager) {
			_entity_manager->setPlayer(_player);
			_entity_manager->clearNonPlayerEntities();
		}

		if (_ui_handler)
			_ui_handler->setPlayer(_player);
	}

} // namespace Nawia::Core
