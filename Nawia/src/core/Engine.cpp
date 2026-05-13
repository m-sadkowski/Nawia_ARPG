#include "Engine.h"

#include <GlobalScaling.h>
#include <Logger.h>
#include <MathUtils.h>
#include <PlayerController.h>

#include <DemoLevel.h>
#include <DemoLevel2.h>
#include <DevLevel.h>
#include <FireballAbility.h>
#include <Level.h>
#include <LevelManager.h>
#include <Map.h>
#include <SoundIds.h>
#include <SwordSlashAbility.h>

#include <string>
#include <utility>
#include <vector>

namespace Nawia::Core {

	namespace {

		constexpr Vector2 k_initial_player_spawn = {0.0f, 0.0f};
		constexpr const char* MENU_MUSIC_PATH = "assets/audio/music/soulfuljamtracks-slavic-folk-308126.mp3";
		constexpr float k_hover_update_interval = 0.05f;
		constexpr float k_hover_mouse_move_threshold_sq = 1.0f;

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

		_audio_manager.playMusic(MENU_MUSIC_PATH, true, 1.f);

		GlobalScaling::setManualScale(_settings.ui_scale);

		_item_database.loadDatabase("assets/data/items.json", _resource_manager);
		Logger::debugLog("Zaladowano baze danych przedmiotow");

		_loottable.loadLootTables("assets/data/loottables.json", _item_database);
		_quest_manager.loadFromJson("assets/data/quests.json");
		_boss_manager.loadFromJson("assets/data/bosses.json");

		_player = Entity::PlayerBuilder(this).setPosition(k_initial_player_spawn).build();
		_player->setAudioManager(&_audio_manager);

		const auto sword_slash_texture = _resource_manager.getTexture("assets/textures/sword_slash.png");
		const auto sword_slash_icon = _resource_manager.getTexture("assets/textures/icons/sword_slash_icon.png");
		_player->addAbility(std::make_shared<Entity::SwordSlashAbility>(sword_slash_texture, sword_slash_icon));

		const auto fireball_hit_texture = _resource_manager.getTexture("assets/textures/fireball_hit.png");
		const auto fireball_icon = _resource_manager.getTexture("assets/textures/icons/fireball_icon.png");
		_player->addAbility(std::make_shared<Entity::FireballAbility>("assets/models/fireball.glb", 0.5f, fireball_hit_texture, fireball_icon));

		_controller = std::make_unique<PlayerController>(this, _player);

		_entity_manager = std::make_unique<EntityManager>(this);
		_entity_manager->addEntity(_player);
		_entity_manager->setPlayer(_player);

		_level_manager = std::make_unique<World::LevelManager>();
		_level_manager->registerLevel(std::make_shared<World::DemoLevel>());
		_level_manager->registerLevel(std::make_shared<World::DemoLevel2>());
		_level_manager->registerLevel(std::make_shared<World::DevLevel>());

		_ui_handler = std::make_unique<UI::UIHandler>();
		_ui_handler->initialize(_player, _entity_manager.get(), _resource_manager, &_quest_manager, &_settings);
		_ui_handler->setLevelManager(_level_manager.get());

		if (_player) {
			const auto sword = _item_database.createItem(1);
			const auto chest = _item_database.createItem(2);
			const auto boots = _item_database.createItem(3);

			if (sword) _player->getBackpack().addItem(sword);
			if (chest) _player->getBackpack().addItem(chest);
			if (boots) _player->getBackpack().addItem(boots);
		}

		_is_running = true;
	}

	Engine::~Engine() {
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
			case GameState::Playing:
				handlePlayingInput();
				break;
		}
	}

	void Engine::handleMenuInput() {
		const UI::MenuAction action = _ui_handler->handleMenuInput();

		if (action == UI::MenuAction::Play) {
			_previous_state = GameState::Menu;
			_ui_handler->openLevelSelect(_level_manager->getRegisteredLevelInfos());
			_game_state = GameState::LevelSelect;
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

		if (selected_level == "BACK") {
			_ui_handler->closeLevelSelect();
			_game_state = GameState::Menu;
		} else if (!selected_level.empty()) {
			_ui_handler->closeLevelSelect();
			_audio_manager.stopMusic();
			_level_manager->changeLevel(selected_level, this);
			_game_state = GameState::Playing;
			_ui_handler->onLevelLoaded();
		}
	}

	void Engine::handlePlayingInput() {
		if (IsKeyPressed(KEY_ESCAPE)) {
			if (_ui_handler->closeOpenWindows())
				return;

			_show_pause_menu = !_show_pause_menu;
			return;
		}

		if (_show_pause_menu) {
			const UI::MenuAction action = _ui_handler->handlePauseMenuInput();
			if (action == UI::MenuAction::Play) {
				_show_pause_menu = false;
			} else if (action == UI::MenuAction::Settings) {
				_previous_state = GameState::Playing;
				_ui_handler->openSettings(_settings);
				_game_state = GameState::SettingsMenu;
				_show_pause_menu = false;
			} else if (action == UI::MenuAction::Exit) {
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

		if (_game_state == GameState::Menu || _game_state == GameState::SettingsMenu || _game_state == GameState::LevelSelect) {
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

		if (_show_pause_menu && _ui_handler)
			_ui_handler->renderPauseMenu();

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
