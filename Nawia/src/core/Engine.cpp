#include "Engine.h"

#include <GlobalScaling.h>
#include <LoadingScreen.h>
#include <Logger.h>
#include <PlayerController.h>

#include <DemoLevel.h>
#include <DevLevel.h>
#include <Entity.h>
#include <FirstLevel.h>
#include <Level.h>
#include <LevelManager.h>
#include <Map.h>
#include <NawiaArenaLevel.h>
#include <SoundIds.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace Nawia::Core {

	namespace {

		constexpr const char* MENU_MUSIC_PATH = "assets/audio/music/soulfuljamtracks-slavic-folk-308126.mp3";
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
		_level_manager->registerLevel(std::make_shared<World::NawiaArenaLevel>());
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

	void Engine::run() {
		while (isRunning()) {
			const float delta_time = GetFrameTime();
			handleInput();
			update(delta_time);
			render();
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
		_combat_telemetry_server.publishAgentCommands(
			_agent_command_interface.getActiveCommands(),
			_agent_command_interface.getCompletedCommands());
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
