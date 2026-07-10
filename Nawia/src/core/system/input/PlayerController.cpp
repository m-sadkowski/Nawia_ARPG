#include "PlayerController.h"

#include <Engine.h>
#include <Player.h>
#include <UIHandler.h>

#include <utility>

namespace Nawia::Core {

	PlayerController::PlayerController(Engine* engine, std::shared_ptr<Entity::Player> player)
		: _engine(engine), _player(std::move(player)) {
	}

	void PlayerController::handleInput(Vector3 mouse_world_pos, const float screen_x, const float screen_y) {
		if (!_engine || !_player) return;

		_last_mouse_x = mouse_world_pos.x;
		_last_mouse_y = mouse_world_pos.z;

		if (_engine->getUIHandler().isInputBlocked()) {
			return;
		}

		if (_player->isControlLocked()) {
			stopCurrentAction();
			return;
		}

		handleMouseInput(mouse_world_pos, screen_x, screen_y);
		handleKeyboardInput(mouse_world_pos, screen_x, screen_y);
	}

	void PlayerController::update(const float dt) {
		if (!_player || _player->isControlLocked()) {
			stopCurrentAction();
			return;
		}

		if (_engine && _engine->getUIHandler().isDialogueOpen()) {
			stopCurrentAction();
			return;
		}

		if (_player->isMovementRooted()) {
			updateRotation();
			return;
		}

		processPendingAction();

		if (processInteraction())
			return;

		if (_target_enemy && _target_enemy->isDead())
			_target_enemy = nullptr;

		processAutoAttack();
		updatePathMovement();
		updateRotation();
	}

	void PlayerController::stopCurrentAction() {
		_target_interactable = nullptr;
		_target_enemy = nullptr;
		_current_path.clear();
		_pending_action = {};
		if (_player)
			_player->stop();
	}

} // namespace Nawia::Core
