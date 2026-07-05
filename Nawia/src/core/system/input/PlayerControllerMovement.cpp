#include "PlayerController.h"

#include <Engine.h>
#include <Logger.h>
#include <Map.h>
#include <Player.h>
#include <UIHandler.h>

#include <string>

namespace Nawia::Core {

	void PlayerController::updatePathMovement() {
		if (_player->isControlLocked())
			return;

		if (_player->isMovementRooted())
			return;

		if (_player->isAnimationLocked())
			return;

		if (!_player->isMoving() && !_current_path.empty()) {
			const Vector2 next_point = _current_path.front();
			const float dx = next_point.x - _player->getCenter().x;
			const float dy = next_point.y - _player->getCenter().y;
			if (dx * dx + dy * dy < 0.10f)
				_current_path.erase(_current_path.begin());

			if (!_current_path.empty())
				_player->moveTo(_current_path.front().x, _current_path.front().y);
		} else if (!_current_path.empty() && _target_enemy) {
			_current_path.clear();
		}
	}

	bool PlayerController::buildPathToWorldPosition(Vector3 desired_world_position) {
		if (!_engine || !_engine->getCurrentMap()) {
			_current_path = {{desired_world_position.x, desired_world_position.z}};
			trimCurrentPathStart();
			return !_current_path.empty();
		}

		const Vector3 start_world_position = _player->getWorldPos3D();
		_current_path = _engine->getCurrentMap()->findPath(start_world_position, desired_world_position);
		Logger::debugLog("PlayerController: sciezka ma " + std::to_string(_current_path.size()) + " punktow");

		trimCurrentPathStart();
		return !_current_path.empty();
	}

	void PlayerController::trimCurrentPathStart() {
		if (_current_path.empty())
			return;

		const Vector2 first_path_point = _current_path.front();
		const float dx = first_path_point.x - _player->getCenter().x;
		const float dy = first_path_point.y - _player->getCenter().y;
		if (dx * dx + dy * dy < 0.1f)
			_current_path.erase(_current_path.begin());
	}

	void PlayerController::moveAlongCurrentPath() {
		if (!_current_path.empty())
			_player->moveTo(_current_path.front().x, _current_path.front().y);
	}

	void PlayerController::updateRotation() const {
		if (_engine && _engine->getUIHandler().isDialogueOpen())
			return;

		if (_player->isControlLocked())
			return;

		if (_target_enemy)
			_player->rotateTowardsCenter(_target_enemy->getCenter().x, _target_enemy->getCenter().y);
		else if (!_player->isMoving() && !_player->isAnimationLocked())
			_player->rotateTowards(_last_mouse_x, _last_mouse_y);
	}

} // namespace Nawia::Core
