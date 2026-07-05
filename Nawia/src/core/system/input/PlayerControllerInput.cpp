#include "PlayerController.h"
#include "PlayerControllerInternal.h"

#include <Ability.h>
#include <Engine.h>
#include <EnemyInterface.h>
#include <Interactable.h>
#include <InteractiveTrigger.h>
#include <Player.h>
#include <UIHandler.h>

namespace Nawia::Core {

	void PlayerController::handleInteractionOnly(Vector3 mouse_world_pos, const float screen_x, const float screen_y) {
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

		if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
			_engine->getPingManager().placePing(_player, mouse_world_pos);
			return;
		}

		if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			return;

		if (const auto entity = _engine->getEntityAt(screen_x, screen_y)) {
			const bool is_trigger = std::dynamic_pointer_cast<Entity::InteractiveTrigger>(entity) != nullptr;
			const auto interactable = std::dynamic_pointer_cast<Entity::Interactable>(entity);
			if (interactable && interactable->canInteract() && !is_trigger) {
				if (_player->isAnimationLocked()) {
					_pending_action = {PendingAction::Type::Interact, 0.0f, 0.0f, 0.0f, -1, entity};
				} else {
					_target_interactable = interactable;
					_target_enemy = nullptr;
					_current_path.clear();
					_pending_action = {};
				}
				return;
			}
		}

		_target_interactable = nullptr;
		_target_enemy = nullptr;
		_current_path.clear();
		_pending_action = {};
		_player->stop();
	}

	void PlayerController::handleMouseInput(Vector3 mouse_world_pos, const float screen_x, const float screen_y) {
		if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
			_engine->getPingManager().placePing(_player, mouse_world_pos);
			return;
		}

		if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

		if (const auto entity = _engine->getEntityAt(screen_x, screen_y)) {
			const bool is_trigger = std::dynamic_pointer_cast<Entity::InteractiveTrigger>(entity) != nullptr;
			const auto interactable = std::dynamic_pointer_cast<Entity::Interactable>(entity);

			if (interactable && interactable->canInteract() && !is_trigger) {
				if (_player->isAnimationLocked()) {
					_pending_action = {PendingAction::Type::Interact, 0.0f, 0.0f, 0.0f, -1, entity};
				} else {
					_target_interactable = interactable;
					_target_enemy = nullptr;
					_current_path.clear();
					_pending_action = {};
				}
				return;
			}

			if (trySelectEnemy(entity))
				return;
		}

		_target_interactable = nullptr;
		handleGroundClick(mouse_world_pos);
	}

	void PlayerController::handleKeyboardInput(Vector3 mouse_world_pos, const float screen_x, const float screen_y) {
		if (_engine->getUIHandler().isInputBlocked()) return;
		if (_player->isControlLocked()) return;

		int ability_index = -1;
		if (IsKeyPressed(KEY_Q)) ability_index = 0;
		if (IsKeyPressed(KEY_W)) ability_index = 1;
		if (IsKeyPressed(KEY_E)) ability_index = 2;
		if (IsKeyPressed(KEY_R)) ability_index = 3;

		if (ability_index == -1) return;

		if (_player->isAnimationLocked())
			queueAbility(ability_index, mouse_world_pos.x, mouse_world_pos.z, screen_x, screen_y);
		else
			castAbility(ability_index, mouse_world_pos.x, mouse_world_pos.z, screen_x, screen_y);
	}

	bool PlayerController::trySelectEnemy(const std::shared_ptr<Entity::Entity>& entity) {
		const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(entity);
		if (!PlayerControllerDetail::isValidEnemyTarget(enemy))
			return false;

		if (_player->isAnimationLocked()) {
			_pending_action = {PendingAction::Type::Move, 0.0f, 0.0f, 0.0f, -1, enemy};
		} else {
			_target_enemy = enemy;
			_pending_action = {};
		}
		return true;
	}

	void PlayerController::handleGroundClick(Vector3 pos) {
		if (_player->isControlLocked()) {
			_pending_action = {};
			_current_path.clear();
			return;
		}

		if (_player->isAnimationLocked()) {
			_pending_action = {PendingAction::Type::Move, pos.x, pos.z, pos.y, -1, std::weak_ptr<Entity::Entity>()};
			return;
		}

		_target_enemy = nullptr;

		if (buildPathToWorldPosition(pos))
			moveAlongCurrentPath();
		else
			_player->stop();

		_pending_action = {};
	}

	void PlayerController::queueAbility(const int index, const float x, const float y, const float screen_x, const float screen_y) {
		_pending_action = {PendingAction::Type::Ability, x, y, 0.0f, index, std::weak_ptr<Entity::Entity>()};

		const auto ability = _player->getAbility(index);
		if (!ability || ability->getTargetType() != Entity::AbilityTargetType::UNIT)
			return;

		const auto target = _engine->getEntityAt(screen_x, screen_y);
		if (!target)
			return;

		const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(target);
		if (!enemy || PlayerControllerDetail::isValidEnemyTarget(enemy))
			_pending_action.target = target;
	}

	void PlayerController::castAbility(const int index, const float x, const float y, const float screen_x, const float screen_y) {
		const auto ability = _player->getAbility(index);
		if (!ability)
			return;

		_pending_action = {};

		switch (ability->getTargetType()) {
			case Entity::AbilityTargetType::UNIT:
				if (const auto target = _engine->getEntityAt(screen_x, screen_y)) {
					if (const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(target);
						PlayerControllerDetail::isValidEnemyTarget(enemy)) {
						_player->setTarget(enemy);
						useAbility(index, enemy->getCenter().x, enemy->getCenter().y);
					}
				}
				break;
			case Entity::AbilityTargetType::POINT:
				useAbility(index, x, y);
				break;
			case Entity::AbilityTargetType::SELF:
			default:
				break;
		}
	}

} // namespace Nawia::Core
