#include "Engine.h"

#include <DevLevel.h>
#include <Level.h>
#include <Map.h>
#include <MathUtils.h>
#include <PlayerController.h>

#include <algorithm>
#include <string>

namespace Nawia::Core {

	namespace {

		constexpr const char* MENU_MUSIC_PATH = "assets/audio/music/soulfuljamtracks-slavic-folk-308126.mp3";
		constexpr float k_hover_update_interval = 0.05f;
		constexpr float k_hover_mouse_move_threshold_sq = 1.0f;

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

} // namespace Nawia::Core
