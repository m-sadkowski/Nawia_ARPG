#include "Engine.h"

#include <json.hpp>

#include <string>
#include <utility>

namespace Nawia::Core {

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

} // namespace Nawia::Core
