#include "Engine.h"

#include <GlobalScaling.h>
#include <Level.h>
#include <LoadingScreen.h>
#include <Map.h>

namespace Nawia::Core {

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
			if (!_show_pause_menu)
				renderPingSelector();
			_ui_handler->render(_camera, &_boss_manager);
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

} // namespace Nawia::Core
