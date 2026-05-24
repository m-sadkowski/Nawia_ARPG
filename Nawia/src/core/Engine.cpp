#include "Engine.h"

#include <GlobalScaling.h>
#include <Logger.h>
#include <MathUtils.h>
#include <PlayerController.h>

#include <DemoLevel.h>
#include <DevLevel.h>
#include <Entity.h>
#include <FireballAbility.h>
#include <Level.h>
#include <LevelManager.h>
#include <Map.h>
#include <SoundIds.h>
#include <SwordSlashAbility.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace Nawia::Core {

	namespace {

		constexpr Vector2 k_initial_player_spawn = {0.0f, 0.0f};
		constexpr const char* MENU_MUSIC_PATH = "assets/audio/music/soulfuljamtracks-slavic-folk-308126.mp3";
		constexpr float k_hover_update_interval = 0.05f;
		constexpr float k_hover_mouse_move_threshold_sq = 1.0f;

		void preloadCommonAnimationData() {
			for (const char* path : {
				"assets/models/animations/anims.glb",
				"assets/models/animations/anims2.glb",
				"assets/models/player/player_head.glb",
				"assets/models/cat_bounce.glb",
				"assets/models/fireball.glb",
				"assets/models/knife.glb",
				"assets/models/bandit_idle.glb",
				"assets/models/bandit_walk_backwards3.glb",
				"assets/models/bandit_throw.glb",
				"assets/models/bandit_death.glb",
				"assets/models/walking_dead_idle.glb",
				"assets/models/walking_dead_walk.glb",
				"assets/models/walking_dead_run.glb",
				"assets/models/walking_dead_attack.glb",
				"assets/models/walking_dead_death.glb",
				"assets/models/walking_dead_scream.glb",
				"assets/models/walking_dead_hit.glb",
				"assets/models/devil_idle.glb",
				"assets/models/devil_walk.glb",
				"assets/models/devil_run.glb",
				"assets/models/devil_attack.glb",
				"assets/models/devil_dead.glb",
				"assets/models/player_idle.glb",
				"assets/models/player_walk.glb",
				"assets/models/player_auto_attack.glb",
				"assets/models/player_knocked.glb",
				"assets/models/dummy_idle.glb",
				"assets/models/dummy_walk.glb",
				"assets/models/dummy_cast_fireball.glb",
				"assets/models/dummy_death.glb"
			}) {
				Entity::Entity::preloadAnimationData(path);
			}
		}

		void preloadCommonModelData(ResourceManager& resource_manager) {
			for (const char* path : {
				"assets/models/fireball.glb",
				"assets/models/knife.glb"
			}) {
				resource_manager.getModel(path);
			}
		}

		void preloadLocationMapData() {
			const std::filesystem::path locations_dir = "assets/data/locations";
			if (!std::filesystem::exists(locations_dir))
				return;

			for (const auto& entry : std::filesystem::directory_iterator(locations_dir)) {
				if (!entry.is_regular_file() || entry.path().extension() != ".json")
					continue;

				const std::string filename = entry.path().filename().generic_string();
				if (filename.rfind("objects_", 0) == 0)
					continue;

				std::ifstream file(entry.path());
				if (!file.is_open())
					continue;

				nlohmann::json data;
				try {
					file >> data;
				} catch (const nlohmann::json::parse_error&) {
					continue;
				}

				if (!data.contains("map") || !data["map"].is_object())
					continue;

				const std::string model = data["map"].value("model", "");
				if (!model.empty() && model != "placeholder")
					Map::preloadMapModel(model);
			}
		}

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

		if (_settings.load())
			SetWindowSize(_settings.resolution.width, _settings.resolution.height);

		_audio_manager.setMasterVolume(_settings.master_volume);
		_audio_manager.setMusicVolume(_settings.music_volume);
		_audio_manager.setEffectsVolume(_settings.effects_volume);
		loadGameplaySounds();
		preloadCommonAnimationData();
		preloadCommonModelData(_resource_manager);
		preloadLocationMapData();

		_audio_manager.playMusic(MENU_MUSIC_PATH, true, 1.f);

		GlobalScaling::setManualScale(_settings.ui_scale);

		_item_database.loadDatabase("assets/data/items.json", _resource_manager);
		Logger::debugLog("Zaladowano baze danych przedmiotow");

		_loottable.loadLootTables("assets/data/loottables.json", _item_database);
		_quest_manager.loadFromJson("assets/data/quests.json");
		_boss_manager.loadFromJson("assets/data/bosses.json");

		_entity_manager = std::make_unique<EntityManager>(this);
		createFreshPlayer(true);

		_level_manager = std::make_unique<World::LevelManager>();
		_level_manager->registerLevel(std::make_shared<World::DemoLevel>());
		_level_manager->registerLevel(std::make_shared<World::DevLevel>());

		_ui_handler = std::make_unique<UI::UIHandler>();
		_ui_handler->initialize(_player, _entity_manager.get(), _resource_manager, &_quest_manager, &_settings);
		_ui_handler->setLevelManager(_level_manager.get());
		_ui_handler->setSaveGameManager(&_save_game_manager);

		_is_running = true;
	}

	Engine::~Engine() {
		if (_level_manager && _level_manager->getCurrentLevel())
			_level_manager->getCurrentLevel()->onExit(this);

		_ui_handler.reset();
		_controller.reset();
		_level_manager.reset();
		_entity_manager.reset();
		_player.reset();
		_boss_manager.clearPreloadedBosses();
		_loottable.clear();
		_item_database.clear();
		Map::clearPreloadedMapModels();
		_resource_manager.clear();

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

	void Engine::createFreshPlayer(const bool grant_starter_items) {
		_player = Entity::PlayerBuilder(this).setPosition(k_initial_player_spawn).build();
		_player->setAudioManager(&_audio_manager);

		const auto sword_slash_icon = _resource_manager.getTexture("assets/textures/icons/sword_slash_icon.png");
		_player->addAbility(std::make_shared<Entity::SwordSlashAbility>(nullptr, sword_slash_icon));

		const auto fireball_icon = _resource_manager.getTexture("assets/textures/icons/fireball_icon.png");
		_player->addAbility(std::make_shared<Entity::FireballAbility>(
			"assets/models/fireball.glb",
			0.5f,
			nullptr,
			fireball_icon,
			&_resource_manager));

		_controller = std::make_unique<PlayerController>(this, _player);

		if (_entity_manager) {
			_entity_manager->setPlayer(_player);
			_entity_manager->clearNonPlayerEntities();
		}

		if (_ui_handler)
			_ui_handler->setPlayer(_player);

		if (!grant_starter_items)
			return;

		for (const int starter_item_id : {1, 2, 3, 8, 9}) {
			if (const auto item = _item_database.createItem(starter_item_id))
				_player->equipItem(item);
		}

		for (const int backpack_item_id : {4, 5, 6, 7, 10, 11, 12}) {
			if (const auto item = _item_database.createItem(backpack_item_id))
				_player->getBackpack().addItem(item);
		}
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
			_player->respawn();
			_entity_manager->addEntity(_player);
			if (_level_manager && _level_manager->getCurrentLevel())
				_level_manager->getCurrentLevel()->prepareForRespawn(this);
			_game_state = GameState::Playing;
		} else if (action == UI::MenuAction::Exit) {
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

		if (selected_slot < 0) {
			_ui_handler->closeSaveSlotMenu();

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

		_ui_handler->closeSaveSlotMenu();

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

		_boss_manager.resetRuntimeState(this);
		_boss_manager.clearDefeatedBosses();
		_quest_manager.resetAll();
		createFreshPlayer(true);

		_audio_manager.stopMusic();
		_level_manager->changeLevel(level_name.empty() ? "Demo" : level_name, this);
		_game_state = GameState::Playing;
		_ui_handler->onLevelLoaded();

		// Zapis tuz po zaladowaniu swiata, zeby checkpointy znaly slot docelowy.
		if (default_slot > 0)
			saveCurrentGame(default_slot);
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
		// `slot == 0` oznacza najnowszy zapis. Zanim podejmiemy proby wczytania,
		// upewniamy sie, ze ten slot w ogole istnieje.
		if (slot == 0 && !_save_game_manager.hasAnySave()) {
			if (_ui_handler)
				_ui_handler->showNotification("Brak zapisu do wczytania.", 3.0f);
			return false;
		}

		createFreshPlayer(false);
		const bool loaded = _save_game_manager.loadGame(*this, slot);
		if (!loaded) {
			if (_ui_handler)
				_ui_handler->showNotification("Nie udalo sie wczytac zapisu.", 3.0f);
			return false;
		}

		_show_pause_menu = false;
		_game_state = GameState::Playing;
		if (_ui_handler) {
			_ui_handler->onLevelLoaded();
			_ui_handler->showNotification("Gra wczytana.", 3.0f);
		}
		return true;
	}

	void Engine::handlePlayingInput() {
		if (IsKeyPressed(KEY_ESCAPE)) {
			if (_ui_handler->closeOpenWindows())
				return;

			_show_pause_menu = !_show_pause_menu;
			return;
		}

		if (_show_pause_menu) {
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

		_ui_handler->handleInput();

		const auto dev_level = dynamic_cast<World::DevLevel*>(_level_manager->getCurrentLevel());
		if (dev_level)
			_camera.handleInput();
		else
			_camera.resetZoom();

		const Vector2 mouse_pos = GetMousePosition();
		const float cursor_plane_height = _player ? _player->getAltitude() : 0.0f;
		const Vector2 fallback = screenToWorldAtHeight(_camera.get(), mouse_pos.x, mouse_pos.y, cursor_plane_height);
		Vector3 mouse_world_pos = {fallback.x, cursor_plane_height, fallback.y};
		const bool needs_precise_ground_hit =
			IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
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

		_level_manager->handleInput(this);
		if (dev_level && dev_level->isTyping())
			return;

		if (_controller)
			_controller->handleInput(mouse_world_pos, mouse_pos.x, mouse_pos.y);
	}

	void Engine::update(const float delta_time) {
		_audio_manager.update();

		if (_game_state == GameState::Menu ||
			_game_state == GameState::SettingsMenu ||
			_game_state == GameState::LevelSelect ||
			_game_state == GameState::SaveSlotSelect) {
			if (_ui_handler) _ui_handler->update(delta_time);
			return;
		}

		if (!_player || !_entity_manager)
			return;

		if (_player->isDead()) {
			if (_boss_manager.isFightActive()) {
				_boss_manager.endBossFight(false, this);
			}
			_game_state = GameState::GameOver;
			return;
		}

		_camera.follow(_player.get());
		_lighting_system.update(_camera.get());
		if (_ui_handler) _ui_handler->update(delta_time);
		_level_manager->update(this, delta_time);
		_controller->update(delta_time);

		_entity_manager->updateEntities(delta_time);
		_entity_manager->handleEntitiesCollisions();
		_quest_manager.update(this);
		_boss_manager.update(this, delta_time);
		collectPendingSpawns();
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
	}

	void Engine::render() const {
		BeginDrawing();
		ClearBackground(Color{30, 30, 35, 255});

		if (_game_state == GameState::Menu && _ui_handler) {
			_ui_handler->renderMainMenu();
		} else if (_game_state == GameState::SettingsMenu && _ui_handler) {
			_ui_handler->renderMainMenu();
			_ui_handler->renderSettingsMenu();
		} else if (_game_state == GameState::LevelSelect && _ui_handler) {
			_ui_handler->renderMainMenu();
			_ui_handler->renderLevelSelectMenu();
		} else if (_game_state == GameState::SaveSlotSelect && _ui_handler) {
			if (_previous_state == GameState::Playing)
				renderGameplay();
			else
				_ui_handler->renderMainMenu();

			_ui_handler->renderSaveSlotMenu();
		} else if (_game_state == GameState::GameOver) {
			renderWorld();
			if (_ui_handler) _ui_handler->renderGameOverScreen();
		} else {
			renderGameplay();
		}

		EndDrawing();
	}

	void Engine::renderWorld() const {
		if (!getCurrentMap() || !_player || !_entity_manager)
			return;

		BeginMode3D(_camera.get());

		_lighting_system.applyToModel(getCurrentMap()->getModel());

		getCurrentMap()->render(_camera.get());
		_entity_manager->renderEntities(_camera.get());

		EndMode3D();
	}

	void Engine::renderGameplay() const {
		if (!getCurrentMap() || !_player || !_entity_manager)
			return;

		renderWorld();

		if (_ui_handler) _ui_handler->render(_camera, &_boss_manager);

		if (_show_pause_menu && _ui_handler) {
			const auto* current_level = _level_manager ? _level_manager->getCurrentLevel() : nullptr;
			_ui_handler->renderPauseMenu(current_level && current_level->allowsSaves());
		}

		_level_manager->renderUI(const_cast<Engine*>(this));
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
