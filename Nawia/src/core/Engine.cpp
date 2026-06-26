#include "Engine.h"

#include <AssetPreloader.h>
#include <GlobalScaling.h>
#include <LoadingScreen.h>
#include <Logger.h>
#include <MathUtils.h>
#include <PlayerController.h>

#include <DemoLevel.h>
#include <DevLevel.h>
#include <Entity.h>
#include <FirstLevel.h>
#include <Level.h>
#include <LevelManager.h>
#include <Map.h>
#include <PlayerAbilityFactory.h>
#include <SoundIds.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <json.hpp>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Nawia::Core {

	namespace {

		constexpr Vector2 k_initial_player_spawn = {0.0f, 0.0f};
		constexpr const char* MENU_MUSIC_PATH = "assets/audio/music/soulfuljamtracks-slavic-folk-308126.mp3";
		constexpr float k_hover_update_interval = 0.05f;
		constexpr float k_hover_mouse_move_threshold_sq = 1.0f;
		constexpr float k_camera_zoom_return_speed = 1.7f;
		constexpr float k_agent_perception_telemetry_interval = 0.25f;

	}

	Engine::Engine() {
		SetTraceLogLevel(LOG_ERROR);
		InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Nawia");
		SetExitKey(0);
		SetTargetFPS(0);
		_audio_manager.initialize();

		_lighting_system.initialize();
		_lighting_system.addLight(System::Renderer::LightingSystem::LIGHT_DIRECTIONAL, {-50.0f, 50.0f, -50.0f}, {0.0f, 0.0f, 0.0f}, WHITE);
		_lighting_system.addLight(System::Renderer::LightingSystem::LIGHT_POINT, {0.0f, 5.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, ORANGE);
		Entity::Entity::setCombatEventBus(&_combat_event_bus);
		if (_combat_telemetry_server.start()) {
			_combat_telemetry_subscription_id = _combat_event_bus.subscribe([this](const Game::CombatEvent& event) {
				_combat_telemetry_server.publish(event);
			});
		}
		else {
			Core::Logger::debugLog("Combat telemetry disabled: " + _combat_telemetry_server.getLastError());
		}

		if (_settings.load())
			SetWindowSize(_settings.resolution.width, _settings.resolution.height);

		_audio_manager.setMasterVolume(_settings.master_volume);
		_audio_manager.setMusicVolume(_settings.music_volume);
		_audio_manager.setEffectsVolume(_settings.effects_volume);
		loadGameplaySounds();

		GlobalScaling::setManualScale(_settings.ui_scale);

		_entity_manager = std::make_unique<EntityManager>(this);
		_level_manager = std::make_unique<World::LevelManager>();
		_level_manager->registerLevel(std::make_shared<World::DemoLevel>());
		_level_manager->registerLevel(std::make_shared<World::FirstLevel>());
		_level_manager->registerLevel(std::make_shared<World::DevLevel>());

		_loading_kind = LoadingKind::Startup;
		_loading_manifest = AssetLoadManifest::buildStartupManifest();
		_loading_asset_index = 0;
		_loading_progress = 0.0f;
		_loading_status = "Przygotowywanie...";
		_loading_title = "Ladowanie gry";
		_game_state = GameState::Loading;

		_is_running = true;
	}

	UI::UIHandler& Engine::getUIHandler() const {
		return *_ui_handler;
	}

	Engine::~Engine() {
		if (_level_manager && _level_manager->getCurrentLevel())
			_level_manager->getCurrentLevel()->onExit(this);

		if (_combat_telemetry_subscription_id != 0) {
			_combat_event_bus.unsubscribe(_combat_telemetry_subscription_id);
			_combat_telemetry_subscription_id = 0;
		}
		_combat_telemetry_server.stop();
		_ui_handler.reset();
		Entity::Entity::setCombatEventBus(nullptr);
		_controller.reset();
		_level_manager.reset();
		_entity_manager.reset();
		_player.reset();
		_boss_manager.clearPreloadedBosses();
		_loottable.clear();
		_item_database.clear();
		Map::clearPreloadedMapModels();
		_resource_manager.clear();
		UI::LoadingScreen::unload();

		CloseWindow();
	}

	bool Engine::isRunning() const {
		return _is_running && !WindowShouldClose();
	}

	std::shared_ptr<Entity::Entity> Engine::getEntityAt(const float screen_x, const float screen_y) const {
		return _entity_manager->getEntityAt(screen_x, screen_y, _camera.get());
	}

	void Engine::spawnEntity(std::shared_ptr<Entity::Entity> new_entity) const {
		if (new_entity)
			_entity_manager->addEntity(std::move(new_entity));
	}

	Map* Engine::getCurrentMap() const {
		if (_level_manager && _level_manager->getCurrentLevel())
			return _level_manager->getCurrentLevel()->getMap();

		return nullptr;
	}

	void Engine::notifyStoryEvent(const std::string& event_id, const Vector2 world_position) {
		if (_level_manager && _level_manager->getCurrentLevel())
			_level_manager->getCurrentLevel()->handleStoryEvent(this, event_id, world_position);
	}

	void Engine::cancelPlayerAction() {
		if (_controller)
			_controller->stopCurrentAction();
		if (_player)
			_player->stop();
	}

	void Engine::requestReturnToMainMenu(std::string notification) {
		_pending_return_to_main_menu = true;
		_pending_return_notification = std::move(notification);
	}

	bool Engine::isLevelInteractionOnly() const {
		const auto* level = _level_manager ? _level_manager->getCurrentLevel() : nullptr;
		return level && level->isInteractionOnly();
	}

	bool Engine::isLevelBlockingControl() const {
		const auto* level = _level_manager ? _level_manager->getCurrentLevel() : nullptr;
		return level && level->blocksPlayerControl();
	}

	float Engine::getLevelCameraZoomMultiplier() const {
		const auto* level = _level_manager ? _level_manager->getCurrentLevel() : nullptr;
		return level ? std::max(0.05f, level->getCameraZoomMultiplier()) : 1.0f;
	}

	float Engine::getLevelCameraTargetHeightMultiplier() const {
		const auto* level = _level_manager ? _level_manager->getCurrentLevel() : nullptr;
		return level ? std::max(0.0f, level->getCameraTargetHeightMultiplier()) : 1.0f;
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

	void Engine::run() {
		while (isRunning()) {
			const float delta_time = GetFrameTime();
			handleInput();
			update(delta_time);
			render();
		}
	}

	void Engine::handleInput() {
		if (!_ui_handler) return;

		switch (_game_state) {
			case GameState::Menu:
				handleMenuInput();
				break;
			case GameState::GameOver:
				handleGameOverInput();
				break;
			case GameState::SettingsMenu:
				handleSettingsInput();
				break;
			case GameState::LevelSelect:
				handleLevelSelectInput();
				break;
			case GameState::SaveSlotSelect:
				handleSaveSlotSelectInput();
				break;
			case GameState::Playing:
				handlePlayingInput();
				break;
		}
	}

	void Engine::handleMenuInput() {
		const UI::MenuAction action = _ui_handler->handleMenuInput();

		if (action == UI::MenuAction::NewGame) {
			_previous_state = GameState::Menu;
			_pending_new_game_level.clear();
			_ui_handler->openLevelSelect(_level_manager->getRegisteredLevelInfos());
			_game_state = GameState::LevelSelect;
		} else if (action == UI::MenuAction::ContinueGame) {
			loadGameFromSlot(0);
		} else if (action == UI::MenuAction::LoadGame) {
			_previous_state = GameState::Menu;
			_ui_handler->openSaveSlotMenu(_save_game_manager.getSaveSlots(), UI::SaveSlotMenu::Mode::Load);
			_game_state = GameState::SaveSlotSelect;
		} else if (action == UI::MenuAction::Settings) {
			_previous_state = GameState::Menu;
			_ui_handler->openSettings(_settings);
			_game_state = GameState::SettingsMenu;
		} else if (action == UI::MenuAction::Authors) {
			_ui_handler->openAuthors();
		} else if (action == UI::MenuAction::Exit) {
			_is_running = false;
		}
	}

	void Engine::handleGameOverInput() {
		const UI::MenuAction action = _ui_handler->handleGameOverInput();
		if (action == UI::MenuAction::Respawn) {
			const bool boss_retry = _boss_manager.isFightActive();
			if (boss_retry)
				_boss_manager.retryActiveBossFight(this);
			_player->respawn();
			_entity_manager->addEntity(_player);
			if (!boss_retry && _level_manager && _level_manager->getCurrentLevel())
				_level_manager->getCurrentLevel()->prepareForRespawn(this);
			_game_state = GameState::Playing;
		} else if (action == UI::MenuAction::Exit) {
			if (_boss_manager.isFightActive())
				_boss_manager.endBossFight(false, this);
			_audio_manager.playMusic(MENU_MUSIC_PATH, true, 0.45f);
			_game_state = GameState::Menu;
		}
	}

	void Engine::handleSettingsInput() {
		if (IsKeyPressed(KEY_ESCAPE)) {
			_ui_handler->closeSettingsMenu();
			_game_state = _previous_state;
			_show_pause_menu = (_previous_state == GameState::Playing);
			return;
		}

		const UI::MenuAction action = _ui_handler->handleSettingsInput();
		if (action == UI::MenuAction::Play) {
			_game_state = _previous_state;
			_show_pause_menu = (_previous_state == GameState::Playing);
			return;
		}

		if (_ui_handler->wereSettingsApplied()) {
			applySettings(_ui_handler->getAppliedSettings());
			_ui_handler->closeSettingsMenu();
			_game_state = _previous_state;
			_show_pause_menu = (_previous_state == GameState::Playing);
		}
	}

	void Engine::handleLevelSelectInput() {
		const std::string selected_level = _ui_handler->handleLevelSelectInput();
		if (selected_level.empty())
			return;

		_ui_handler->closeLevelSelect();

		if (selected_level == "BACK") {
			_pending_new_game_level.clear();
			_game_state = GameState::Menu;
			return;
		}

		// Poziomy bez systemu zapisu (np. kreator) startuja od razu, z pomijaniem
		// wyboru slotu i auto-zapisu po zaladowaniu swiata.
		const auto level_infos = _level_manager->getRegisteredLevelInfos();
		const auto level_info = std::ranges::find_if(level_infos, [&](const World::LevelInfo& info) {
			return info.name == selected_level;
		});
		if (level_info != level_infos.end() && !level_info->allows_saves) {
			_pending_new_game_level.clear();
			startNewGame(selected_level, 0);
			return;
		}

		// Po wybraniu poziomu fabularnego prosimy o slot startowy dla nowej gry.
		_pending_new_game_level = selected_level;
		_ui_handler->openSaveSlotMenu(_save_game_manager.getSaveSlots(), UI::SaveSlotMenu::Mode::SelectDefault);
		_game_state = GameState::SaveSlotSelect;
	}

	void Engine::handleSaveSlotSelectInput() {
		const int selected_slot = _ui_handler->handleSaveSlotInput();
		if (selected_slot == 0)
			return;

		const bool opened_from_game = _previous_state == GameState::Playing;
		const UI::SaveSlotMenu::Mode mode = _ui_handler->getSaveSlotMenuMode();
		_ui_handler->closeSaveSlotMenu();

		if (selected_slot < 0) {
			// Anulowanie wyboru slotu w nowej grze cofa nas do wyboru poziomu.
			if (mode == UI::SaveSlotMenu::Mode::SelectDefault) {
				_ui_handler->openLevelSelect(_level_manager->getRegisteredLevelInfos());
				_game_state = GameState::LevelSelect;
				return;
			}

			_game_state = opened_from_game ? GameState::Playing : GameState::Menu;
			_show_pause_menu = opened_from_game;
			return;
		}

		switch (mode) {
			case UI::SaveSlotMenu::Mode::Save:
				saveCurrentGame(selected_slot);
				_game_state = GameState::Playing;
				_show_pause_menu = opened_from_game;
				return;
			case UI::SaveSlotMenu::Mode::SelectDefault:
				startNewGame(_pending_new_game_level, selected_slot);
				_pending_new_game_level.clear();
				return;
			case UI::SaveSlotMenu::Mode::Load:
			default:
				loadGameFromSlot(selected_slot);
				return;
		}
	}

	void Engine::startNewGame(const std::string& level_name, const int default_slot) {
		_save_game_manager.clearActiveSlot();
		_show_pause_menu = false;
		_previous_state = GameState::Menu;
		_has_pending_save = false;
		_pending_save_state = {};

		_boss_manager.resetRuntimeState(this);
		_boss_manager.clearDefeatedBosses();
		_quest_manager.resetAll();
		createFreshPlayer();

		queueLevelLoad(level_name, "", true, default_slot);
	}

	bool Engine::saveCurrentGame(const int slot) {
		const bool saved = _save_game_manager.saveGame(*this, slot);
		if (_ui_handler)
			_ui_handler->showNotification(saved ? "Gra zapisana." : "Nie udalo sie zapisac gry.", 3.0f);

		return saved;
	}

	bool Engine::saveGameToActiveSlot() {
		const int active_slot = _save_game_manager.getActiveSlot();
		if (active_slot <= 0)
			return false;

		return saveCurrentGame(active_slot);
	}

	bool Engine::loadGameFromSlot(const int slot) {
		if (slot == 0 && !_save_game_manager.hasAnySave()) {
			if (_ui_handler)
				_ui_handler->showNotification("Brak zapisu do wczytania.", 3.0f);
			return false;
		}

		nlohmann::json save_state;
		int resolved_slot = 0;
		if (!_save_game_manager.tryReadSave(slot, save_state, resolved_slot)) {
			if (_ui_handler)
				_ui_handler->showNotification("Nie udalo sie wczytac zapisu.", 3.0f);
			return false;
		}

		const std::string current_level_name = save_state.value("current_level", "");
		if (current_level_name.empty()) {
			if (_ui_handler)
				_ui_handler->showNotification("Nie udalo sie wczytac zapisu.", 3.0f);
			return false;
		}

		_show_pause_menu = false;
		_previous_state = GameState::Menu;
		_has_pending_save = true;
		_pending_save_state = std::move(save_state);
		_pending_save_slot = resolved_slot;

		_boss_manager.resetRuntimeState(this);
		_boss_manager.clearDefeatedBosses();
		_quest_manager.resetAll();
		createFreshPlayer();

		const std::string initial_location = _pending_save_state.value("current_location", "");
		queueLevelLoad(current_level_name, initial_location, false, 0);
		return true;
	}

	void Engine::handlePlayingInput() {
		if (isLevelBlockingControl()) {
			_show_pause_menu = false;
			if (_player)
				_player->stop();
			if (_ui_handler && _ui_handler->isDialogueOpen())
				_ui_handler->handleInput();
			return;
		}

		if (!isLevelInteractionOnly() && IsKeyPressed(KEY_ESCAPE)) {
			if (_ui_handler->closeOpenWindows())
				return;

			_show_pause_menu = !_show_pause_menu;
			return;
		}

		if (!isLevelInteractionOnly() && _show_pause_menu) {
			const auto* current_level = _level_manager ? _level_manager->getCurrentLevel() : nullptr;
			const bool saves_enabled = current_level && current_level->allowsSaves();

			const UI::MenuAction action = _ui_handler->handlePauseMenuInput(saves_enabled);
			if (action == UI::MenuAction::Play) {
				_show_pause_menu = false;
			} else if (action == UI::MenuAction::SaveGame) {
				_previous_state = GameState::Playing;
				_ui_handler->openSaveSlotMenu(_save_game_manager.getSaveSlots(), UI::SaveSlotMenu::Mode::Save);
				_game_state = GameState::SaveSlotSelect;
				_show_pause_menu = false;
			} else if (action == UI::MenuAction::LoadGame) {
				_previous_state = GameState::Playing;
				_ui_handler->openSaveSlotMenu(_save_game_manager.getSaveSlots(), UI::SaveSlotMenu::Mode::Load);
				_game_state = GameState::SaveSlotSelect;
				_show_pause_menu = false;
			} else if (action == UI::MenuAction::Settings) {
				_previous_state = GameState::Playing;
				_ui_handler->openSettings(_settings);
				_game_state = GameState::SettingsMenu;
				_show_pause_menu = false;
			} else if (action == UI::MenuAction::MainMenu || action == UI::MenuAction::Exit) {
				_audio_manager.playMusic(MENU_MUSIC_PATH, true, 1.f);
				_game_state = GameState::Menu;
				_show_pause_menu = false;
			}
			return;
		}

		const bool dialogue_was_open = _ui_handler->isDialogueOpen();
		_ui_handler->handleInput();
		if (dialogue_was_open || isLevelBlockingControl())
			return;

		const auto dev_level = dynamic_cast<World::DevLevel*>(_level_manager->getCurrentLevel());
		if (dev_level)
			_camera.handleInput();

		const Vector2 mouse_pos = GetMousePosition();
		const float cursor_plane_height = _player ? _player->getAltitude() : 0.0f;
		const Vector2 fallback = screenToWorldAtHeight(_camera.get(), mouse_pos.x, mouse_pos.y, cursor_plane_height);
		Vector3 mouse_world_pos = {fallback.x, cursor_plane_height, fallback.y};
		const bool needs_precise_ground_hit =
			IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
			IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) ||
			IsKeyPressed(KEY_Q) ||
			IsKeyPressed(KEY_W) ||
			IsKeyPressed(KEY_E) ||
			IsKeyPressed(KEY_R);

		if (needs_precise_ground_hit && getCurrentMap()) {
			const Ray ray = GetMouseRay(mouse_pos, _camera.get());
			const RayCollision collision = getCurrentMap()->getRayCollision(ray);
			if (collision.hit)
				mouse_world_pos = collision.point;
		}

		_hover_update_timer -= GetFrameTime();
		const float hover_dx = mouse_pos.x - _last_hover_mouse_pos.x;
		const float hover_dy = mouse_pos.y - _last_hover_mouse_pos.y;
		const bool hover_mouse_moved = hover_dx * hover_dx + hover_dy * hover_dy >= k_hover_mouse_move_threshold_sq;
		if (hover_mouse_moved || _hover_update_timer <= 0.0f) {
			_entity_manager->updateHoverState(mouse_pos.x, mouse_pos.y, _camera.get());
			_last_hover_mouse_pos = mouse_pos;
			_hover_update_timer = k_hover_update_interval;
		}

		// Aktualizacja stanu kursora na podstawie hoverowanej encji.
		const auto hovered_entity = _entity_manager->getHoveredEntity();
		const bool level_blocks_control = _level_manager->getCurrentLevel() && _level_manager->getCurrentLevel()->blocksPlayerControl();
		
		// Pokazujemy kursor interakcji z encja tylko jesli UI nie blokuje wejscia (np. otwarty dialog)
		// i poziom nie blokuje kontroli (np. intro).
		if (hovered_entity && !_ui_handler->isInputBlocked() && !level_blocks_control)
			_custom_cursor.setState(UI::CursorState::Interact);
		else
			_custom_cursor.setState(UI::CursorState::Default);

		_level_manager->handleInput(this);
		if (dev_level && dev_level->isTyping())
			return;

		if (!_ui_handler->isInputBlocked()) {
			if (IsKeyPressed(KEY_TWO))
				_ping_manager.selectType(Game::MapPingType::Info);
			if (IsKeyPressed(KEY_THREE))
				_ping_manager.selectType(Game::MapPingType::Threat);

			const float mouse_wheel_move = GetMouseWheelMove();
			if (mouse_wheel_move != 0.0f)
				_ping_manager.cycleSelectedType(mouse_wheel_move > 0.0f ? 1 : -1);
		}

		if (!isLevelInteractionOnly() && !_ui_handler->isInputBlocked() && IsKeyPressed(KEY_ONE) && _player)
			(void)_player->startConsumeFood();

		if (_controller) {
			if (isLevelInteractionOnly())
				_controller->handleInteractionOnly(mouse_world_pos, mouse_pos.x, mouse_pos.y);
			else
				_controller->handleInput(mouse_world_pos, mouse_pos.x, mouse_pos.y);
		}
	}

	void Engine::update(const float delta_time) {
		_audio_manager.update();
		_combat_event_bus.update(delta_time);

		if (_game_state == GameState::Loading) {
			processLoading();
			return;
		}

		if (_pending_return_to_main_menu && (!_ui_handler || !_ui_handler->isDialogueOpen())) {
			_pending_return_to_main_menu = false;
			if (!_pending_return_notification.empty())
				_ui_handler->showNotification(_pending_return_notification, 6.0f);
			_pending_return_notification.clear();
			_audio_manager.playMusic(MENU_MUSIC_PATH, true, 0.45f);
			_show_pause_menu = false;
			_game_state = GameState::Menu;
			return;
		}

		if (_game_state == GameState::Menu ||
			_game_state == GameState::SettingsMenu ||
			_game_state == GameState::LevelSelect ||
			_game_state == GameState::SaveSlotSelect) {
			_custom_cursor.setState(UI::CursorState::Default);
			if (_ui_handler) _ui_handler->update(delta_time);
			return;
		}

		if (!_player || !_entity_manager)
			return;

		if (_player->isDead()) {
			_game_state = GameState::GameOver;
			return;
		}

		if (!dynamic_cast<World::DevLevel*>(_level_manager->getCurrentLevel())) {
			const float level_zoom_multiplier = getLevelCameraZoomMultiplier();
			const float target_zoom = _gameplay_camera_zoom * level_zoom_multiplier;
			if (std::abs(level_zoom_multiplier - 1.0f) > 0.001f) {
				_current_camera_zoom = target_zoom;
			} else {
				const float zoom_t = std::clamp(delta_time * k_camera_zoom_return_speed, 0.0f, 1.0f);
				_current_camera_zoom += (target_zoom - _current_camera_zoom) * zoom_t;
			}
			_camera.resetZoom(_current_camera_zoom);

			const float target_height_multiplier = getLevelCameraTargetHeightMultiplier();
			if (std::abs(target_height_multiplier - 1.0f) > 0.001f) {
				_current_camera_target_height_multiplier = target_height_multiplier;
			} else {
				const float height_t = std::clamp(delta_time * k_camera_zoom_return_speed, 0.0f, 1.0f);
				_current_camera_target_height_multiplier +=
					(1.0f - _current_camera_target_height_multiplier) * height_t;
			}
		}

		_camera.follow(_player.get(), _current_camera_target_height_multiplier);
		_lighting_system.update(_camera.get());
		if (_ui_handler) _ui_handler->update(delta_time);
		_ping_manager.update(delta_time);
		_level_manager->update(this, delta_time);
		if (isLevelBlockingControl()) {
			_agent_command_interface.update(*this, *_entity_manager, delta_time);
			_entity_manager->updateEntities(delta_time);
			collectPendingSpawns();
			updateAgentPerceptionTelemetry(delta_time);
			return;
		}
		_controller->update(delta_time);

		_agent_command_interface.update(*this, *_entity_manager, delta_time);
		_entity_manager->updateEntities(delta_time);
		_entity_manager->handleEntitiesCollisions();
		_quest_manager.update(this);
		_boss_manager.update(this, delta_time);
		collectPendingSpawns();
		updateAgentPerceptionTelemetry(delta_time);
	}

	void Engine::collectPendingSpawns() {
		std::vector<std::shared_ptr<Entity::Entity>> new_spawns;
		for (const auto& entity : _entity_manager->getEntities()) {
			const auto& spawns = entity->getPendingSpawns();
			if (!spawns.empty()) {
				new_spawns.insert(new_spawns.end(), spawns.begin(), spawns.end());
				entity->clearPendingSpawns();
			}
		}

		for (const auto& spawn : new_spawns)
			spawnEntity(spawn);
	}

	void Engine::updateAgentPerceptionTelemetry(const float delta_time) {
		if (!_entity_manager)
			return;

		_agent_perception_system.update(*_entity_manager, _combat_event_bus, _ping_manager);
		if (!_combat_telemetry_server.isRunning())
			return;

		_agent_perception_telemetry_timer -= delta_time;
		if (_agent_perception_telemetry_timer > 0.0f)
			return;

		_agent_perception_telemetry_timer = k_agent_perception_telemetry_interval;
		for (const auto& snapshot : _agent_perception_system.getSnapshots())
			_combat_telemetry_server.publishAgentPerception(snapshot);
	}

	void Engine::loadGameplaySounds() {
		_audio_manager.loadSound(Audio::SoundId::ZombieScream, Audio::SoundPath::ZombieScream);
		_audio_manager.loadSound(Audio::SoundId::ZombieDeath, Audio::SoundPath::ZombieDeath);
		_audio_manager.loadSound(Audio::SoundId::ZombieAmbient, Audio::SoundPath::ZombieAmbient);
		_audio_manager.loadSound(Audio::SoundId::SwordSlash, Audio::SoundPath::SwordSlash);
		_audio_manager.loadSound(Audio::SoundId::FireballCast, Audio::SoundPath::FireballCast);
		_audio_manager.loadSound(Audio::SoundId::DevilDeath, Audio::SoundPath::DevilDeath);
		_audio_manager.loadSound(Audio::SoundId::DevilDash, Audio::SoundPath::DevilDash);
		_audio_manager.loadSound(Audio::SoundId::DevilAggro, Audio::SoundPath::DevilAggro);
		_audio_manager.loadSound(Audio::SoundId::DevilPunch, Audio::SoundPath::DevilPunch);
		_audio_manager.loadSound(Audio::SoundId::DevilStep, Audio::SoundPath::DevilStep);
		_audio_manager.loadSound(Audio::SoundId::ChestOpen, Audio::SoundPath::ChestOpen);
		_audio_manager.loadSound(Audio::SoundId::DevilDashHit, Audio::SoundPath::DevilDashHit);
		_audio_manager.loadSound(Audio::SoundId::ItemEquip, Audio::SoundPath::ItemEquip);
		_audio_manager.loadSound(Audio::SoundId::FireballHit, Audio::SoundPath::FireballHit);
		_audio_manager.loadSound(Audio::SoundId::PlayerHurt, Audio::SoundPath::PlayerHurt);
		_audio_manager.loadSound(Audio::SoundId::HumanDeath, Audio::SoundPath::HumanDeath);
		_audio_manager.loadSound(Audio::SoundId::KnifeThrow, Audio::SoundPath::KnifeThrow);
		_audio_manager.loadSound(Audio::SoundId::CatMeow, Audio::SoundPath::CatMeow);
		_audio_manager.loadSound(Audio::SoundId::FrogSound, Audio::SoundPath::FrogSound);
		_audio_manager.loadSound(Audio::SoundId::SpiderWebShot, Audio::SoundPath::SpiderWebShot);
		_audio_manager.loadSound(Audio::SoundId::SpiderMeleeAttack, Audio::SoundPath::SpiderMeleeAttack);
		_audio_manager.loadSound(Audio::SoundId::MiniMushroomAttack, Audio::SoundPath::MiniMushroomAttack);
		_audio_manager.loadSound(Audio::SoundId::MiniMushroomWormExit, Audio::SoundPath::MiniMushroomWormExit);
		_audio_manager.loadSound(Audio::SoundId::PlayerEatSupplies, Audio::SoundPath::PlayerEatSupplies);
	}

	void Engine::render() const {
		BeginDrawing();
		ClearBackground(Color{30, 30, 35, 255});

		if (_game_state == GameState::Loading) {
			UI::LoadingScreen::render(_loading_progress, _loading_status, _loading_title);
			EndDrawing();
			return;
		}

		if (_game_state == GameState::Menu && _ui_handler) {
			_ui_handler->renderMainMenu();
		} else if (_game_state == GameState::SettingsMenu && _ui_handler) {
			_ui_handler->renderMainMenu();
			_ui_handler->renderSettingsMenu();
		} else if (_game_state == GameState::LevelSelect && _ui_handler) {
			_ui_handler->renderMainMenu();
			_ui_handler->renderLevelSelectMenu();
		} else if (_game_state == GameState::SaveSlotSelect && _ui_handler) {
			// Wybor slotu z menu glownego to osobny ekran - rysujemy samo tlo
			// menu zamiast przyciskow "Nowa gra" itp., zeby nie przebijaly sie
			// spod polprzezroczystego dimm-u SaveSlotMenu. Z pauzy zachowujemy
			// widoczna rozgrywke w tle.
			if (_previous_state == GameState::Playing)
				renderGameplay();
			else
				_ui_handler->drawSharedMenuBackground();

			_ui_handler->renderSaveSlotMenu();
		} else if (_game_state == GameState::GameOver) {
			renderWorld();
			if (_ui_handler) _ui_handler->renderGameOverScreen();
		} else {
			renderGameplay();
		}

		_custom_cursor.render();
		EndDrawing();
	}

	void Engine::renderWorld() const {
		if (!getCurrentMap() || !_player || !_entity_manager)
			return;

		BeginMode3D(_camera.get());

		_lighting_system.applyToModel(getCurrentMap()->getModel());

		getCurrentMap()->render(_camera.get());
		_ping_manager.render(_camera.get());
		_entity_manager->renderEntities(_camera.get());

		EndMode3D();
	}

	void Engine::renderGameplay() const {
		if (!getCurrentMap() || !_player || !_entity_manager)
			return;

		renderWorld();
		renderGameplayVignetteOverlay();
		if (_level_manager && _level_manager->getCurrentLevel())
			_level_manager->getCurrentLevel()->renderOverlay(const_cast<Engine*>(this));

		if (isLevelBlockingControl()) {
			if (_ui_handler)
				_ui_handler->renderDialogueOnly();
			return;
		}

		if (_ui_handler) {
			_ui_handler->render(_camera, &_boss_manager);
			if (!_show_pause_menu)
				renderPingSelector();
		}

		if (_show_pause_menu && _ui_handler) {
			const auto* current_level = _level_manager ? _level_manager->getCurrentLevel() : nullptr;
			_ui_handler->renderPauseMenu(current_level && current_level->allowsSaves());
		}

		_level_manager->renderUI(const_cast<Engine*>(this));
	}

	void Engine::renderPingSelector() const {
		const float frame_width = GlobalScaling::scaled(520.0f);
		const float frame_height = GlobalScaling::scaled(111.0f);
		const float frame_x = (static_cast<float>(GetScreenWidth()) - frame_width) * 0.5f;
		const float frame_y = static_cast<float>(GetScreenHeight()) - GlobalScaling::scaled(126.0f);
		const float icon_size = frame_height * 0.55f;
		const float food_center_x = frame_x + frame_width * 0.5f;
		const float food_y = frame_y + frame_height * 0.53f - icon_size * 0.5f;
		const float radius = GlobalScaling::scaled(6.0f);
		const Vector2 center = {
			food_center_x,
			food_y - GlobalScaling::scaled(34.0f)
		};
		const Color color = Game::getPingColor(_ping_manager.getSelectedType());

		DrawCircleV(center, radius + GlobalScaling::scaled(3.0f), Fade(BLACK, 0.72f));
		DrawCircleV(center, radius + GlobalScaling::scaled(1.5f), Fade(WHITE, 0.18f));
		DrawCircleV(center, radius, color);
	}

	void Engine::renderGameplayVignetteOverlay() const {
		const int width = GetScreenWidth();
		const int height = GetScreenHeight();
		const int edge_x = static_cast<int>(static_cast<float>(width) * 0.18f);
		const int edge_y = static_cast<int>(static_cast<float>(height) * 0.18f);

		DrawRectangleGradientH(0, 0, edge_x, height, Fade(BLACK, 0.24f), Fade(BLACK, 0.0f));
		DrawRectangleGradientH(width - edge_x, 0, edge_x, height, Fade(BLACK, 0.0f), Fade(BLACK, 0.24f));
		DrawRectangleGradientV(0, 0, width, edge_y, Fade(BLACK, 0.20f), Fade(BLACK, 0.0f));
		DrawRectangleGradientV(0, height - edge_y, width, edge_y, Fade(BLACK, 0.0f), Fade(BLACK, 0.22f));
	}

	void Engine::applySettings(const Settings& new_settings) {
		_settings = new_settings;

		if (IsWindowFullscreen()) {
			if (!_settings.fullscreen)
				ToggleFullscreen();
			else
				SetWindowSize(_settings.resolution.width, _settings.resolution.height);
		} else if (_settings.fullscreen) {
			SetWindowSize(_settings.resolution.width, _settings.resolution.height);
			ToggleFullscreen();
		} else {
			SetWindowSize(_settings.resolution.width, _settings.resolution.height);
		}

		GlobalScaling::setManualScale(_settings.ui_scale);
		_audio_manager.setMasterVolume(_settings.master_volume);
		_audio_manager.setMusicVolume(_settings.music_volume);
		_audio_manager.setEffectsVolume(_settings.effects_volume);

		if (_settings.save())
			Logger::debugLog("Zapisano ustawienia.");
	}

} // namespace Nawia::Core
