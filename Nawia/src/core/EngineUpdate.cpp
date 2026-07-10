#include "Engine.h"

#include <DevLevel.h>
#include <Level.h>
#include <PlayerController.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace Nawia::Core {

	namespace {

		constexpr const char* MENU_MUSIC_PATH = "assets/audio/music/soulfuljamtracks-slavic-folk-308126.mp3";
		constexpr float k_camera_zoom_return_speed = 1.7f;
		constexpr float k_agent_perception_telemetry_interval = 0.25f;

	}

	float Engine::getLevelCameraZoomMultiplier() const {
		const auto* level = _level_manager ? _level_manager->getCurrentLevel() : nullptr;
		return level ? std::max(0.05f, level->getCameraZoomMultiplier()) : 1.0f;
	}

	float Engine::getLevelCameraTargetHeightMultiplier() const {
		const auto* level = _level_manager ? _level_manager->getCurrentLevel() : nullptr;
		return level ? std::max(0.0f, level->getCameraTargetHeightMultiplier()) : 1.0f;
	}

	void Engine::update(const float delta_time) {
		updateAlwaysOnSystems(delta_time);

		if (handleLoadingUpdate())
			return;

		if (handlePendingMainMenuReturn())
			return;

		if (updateMenuLikeState(delta_time))
			return;

		if (!ensureGameplayReady())
			return;

		updateGameplayCamera(delta_time);
		updateGameplayPresentation(delta_time);

		if (isLevelBlockingControl()) {
			updateControlBlockedGameplay(delta_time);
			return;
		}

		updateActiveGameplay(delta_time);
	}

	void Engine::updateAlwaysOnSystems(const float delta_time) {
		_audio_manager.update();
		_combat_event_bus.update(delta_time);
	}

	bool Engine::handleLoadingUpdate() {
		if (_game_state != GameState::Loading)
			return false;

		processLoading();
		return true;
	}

	bool Engine::handlePendingMainMenuReturn() {
		if (!_pending_return_to_main_menu || (_ui_handler && _ui_handler->isDialogueOpen()))
			return false;

		_pending_return_to_main_menu = false;
		if (!_pending_return_notification.empty())
			_ui_handler->showNotification(_pending_return_notification, 6.0f);
		_pending_return_notification.clear();
		_audio_manager.playMusic(MENU_MUSIC_PATH, true, 0.45f);
		_show_pause_menu = false;
		_game_state = GameState::Menu;
		return true;
	}

	bool Engine::updateMenuLikeState(const float delta_time) {
		if (_game_state != GameState::Menu &&
			_game_state != GameState::SettingsMenu &&
			_game_state != GameState::LevelSelect &&
			_game_state != GameState::SaveSlotSelect) {
			return false;
		}

		_custom_cursor.setState(UI::CursorState::Default);
		if (_ui_handler)
			_ui_handler->update(delta_time);
		return true;
	}

	bool Engine::ensureGameplayReady() {
		if (!_player || !_entity_manager)
			return false;

		if (_player->isDead()) {
			_game_state = GameState::GameOver;
			return false;
		}

		return true;
	}

	void Engine::updateGameplayCamera(const float delta_time) {
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
	}

	void Engine::updateGameplayPresentation(const float delta_time) {
		_lighting_system.update(_camera.get());
		if (_ui_handler)
			_ui_handler->update(delta_time);
		_ping_manager.update(delta_time);
		_level_manager->update(this, delta_time);
	}

	void Engine::updateControlBlockedGameplay(const float delta_time) {
		_agent_command_interface.update(*this, *_entity_manager, delta_time);
		_entity_manager->updateEntities(delta_time);
		collectPendingSpawns();
		updateAgentPerceptionTelemetry(delta_time);
	}

	void Engine::updateActiveGameplay(const float delta_time) {
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

} // namespace Nawia::Core
