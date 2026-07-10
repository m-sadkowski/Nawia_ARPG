#include "PlayerController.h"
#include "PlayerControllerInternal.h"

#include <Ability.h>
#include <Engine.h>
#include <EnemyInterface.h>
#include <Interactable.h>
#include <Logger.h>
#include <Player.h>

#include <cmath>
#include <string>

namespace Nawia::Core {

	void PlayerController::useAbility(const int index, const float target_x, const float target_y) const {
		if (_player->isControlLocked())
			return;

		const auto ability = _player->getAbility(index);
		if (!ability || !ability->isReady())
			return;

		_player->stop();
		_player->rotateTowards(target_x, target_y);

		if (auto effect = ability->cast(target_x, target_y)) {
			Logger::debugLog("PlayerController: uzyto umiejetnosci " + std::to_string(index) +
				", cel=(" + std::to_string(target_x) + ", " + std::to_string(target_y) + ")");
			_engine->spawnEntity(effect);
		}
	}

	void PlayerController::processPendingAction() {
		if (_player->isControlLocked()) {
			_pending_action = {};
			return;
		}

		if (_player->isAnimationLocked() || _pending_action.type == PendingAction::Type::None)
			return;

		if (_pending_action.type == PendingAction::Type::Move)
			processPendingMove();
		else if (_pending_action.type == PendingAction::Type::Ability)
			processPendingAbility();
		else if (_pending_action.type == PendingAction::Type::Interact)
			_target_interactable = std::dynamic_pointer_cast<Entity::Interactable>(_pending_action.target.lock());

		_pending_action = {};
	}

	void PlayerController::processAutoAttack() {
		if (_player->isControlLocked()) return;
		if (_player->isAnimationLocked()) return;

		const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(_target_enemy);
		if (!PlayerControllerDetail::isValidEnemyTarget(enemy)) {
			_target_enemy = nullptr;
			return;
		}

		const Vector2 player_center = _player->getCenter();
		const Vector2 enemy_center = enemy->getCenter();
		const float dx = enemy_center.x - player_center.x;
		const float dy = enemy_center.y - player_center.y;
		const float distance_sq = dx * dx + dy * dy;

		constexpr int auto_attack_index = 0;
		const auto auto_attack = _player->getAbility(auto_attack_index);
		if (!auto_attack)
			return;

		updateCombatMovement(distance_sq, auto_attack->getCastRange());
	}

	void PlayerController::processPendingMove() {
		_target_enemy = nullptr;

		if (const auto target = _pending_action.target.lock()) {
			if (const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(target))
				_target_enemy = enemy;
			return;
		}

		const Vector3 pending_world_position = {_pending_action.x, _pending_action.world_height, _pending_action.y};
		if (buildPathToWorldPosition(pending_world_position))
			moveAlongCurrentPath();
		else
			_player->stop();
	}

	void PlayerController::processPendingAbility() const {
		const auto ability = _player->getAbility(_pending_action.ability_index);
		if (!ability)
			return;

		if (ability->getTargetType() == Entity::AbilityTargetType::UNIT) {
			const auto target = _pending_action.target.lock();
			const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(target);
			if (PlayerControllerDetail::isValidEnemyTarget(enemy)) {
				_player->setTarget(enemy);
				useAbility(_pending_action.ability_index, enemy->getCenter().x, enemy->getCenter().y);
			}
			return;
		}

		useAbility(_pending_action.ability_index, _pending_action.x, _pending_action.y);
	}

	void PlayerController::updateCombatMovement(const float distance_sq, const float attack_range) {
		constexpr float hysteresis = 0.5f;
		constexpr int auto_attack_index = 0;
		const float hit_range = (attack_range * 0.5f) + hysteresis;

		if (distance_sq > hit_range * hit_range) {
			_player->moveTo(_target_enemy->getCenter().x, _target_enemy->getCenter().y);
			return;
		}

		_player->rotateTowards(_target_enemy->getCenter().x, _target_enemy->getCenter().y);
		_player->stop();
		useAbility(auto_attack_index, _target_enemy->getCenter().x, _target_enemy->getCenter().y);
	}

} // namespace Nawia::Core
