#include "PlayerController.h"

#include <Ability.h>
#include <Cat.h>
#include <Engine.h>
#include <EnemyInterface.h>
#include <Interactable.h>
#include <InteractiveClickable.h>
#include <InteractiveTrigger.h>
#include <Logger.h>
#include <Map.h>
#include <Player.h>
#include <UIHandler.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace Nawia::Core {

	namespace {

		float getHorizontalDistanceToBoxSq(const Entity::Entity& entity, const Vector2 position) {
			const BoundingBox box = entity.getBoundingBox();
			const float closest_x = std::clamp(position.x, box.min.x, box.max.x);
			const float closest_y = std::clamp(position.y, box.min.z, box.max.z);
			const float dx = position.x - closest_x;
			const float dy = position.y - closest_y;
			return dx * dx + dy * dy;
		}

		bool isValidEnemyTarget(const std::shared_ptr<Entity::EnemyInterface>& enemy) {
			return enemy && !enemy->isDead() && enemy->getFaction() != Entity::Faction::None;
		}

	}

	PlayerController::PlayerController(Engine* engine, std::shared_ptr<Entity::Player> player)
		: _engine(engine), _player(std::move(player)) {
	}

	void PlayerController::handleInput(Vector3 mouse_world_pos, const float screen_x, const float screen_y) {
		if (!_engine || !_player) return;

		_last_mouse_x = mouse_world_pos.x;
		_last_mouse_y = mouse_world_pos.z;

		if (_engine->getUIHandler().isInputBlocked()) {
			_player->stop();
			return;
		}

		handleMouseInput(mouse_world_pos, screen_x, screen_y);
		handleKeyboardInput(mouse_world_pos, screen_x, screen_y);
	}

	void PlayerController::update(const float dt) {
		processPendingAction();

		if (processInteraction())
			return;

		if (_target_enemy && _target_enemy->isDead())
			_target_enemy = nullptr;

		processAutoAttack();
		updatePathMovement();
		updateRotation();
	}

	bool PlayerController::processInteraction() {
		if (!_target_interactable)
			return false;

		const auto target_entity = std::dynamic_pointer_cast<Entity::Entity>(_target_interactable);
		if (!target_entity) {
			_target_interactable = nullptr;
			return false;
		}

		const float interaction_range_sq = _target_interactable->getInteractionRange();
		const float distance_sq = getHorizontalDistanceToBoxSq(*target_entity, _player->getCenter());

		if (distance_sq > interaction_range_sq) {
			if (!_current_path.empty() || _player->isMoving()) {
				updatePathMovement();
			} else if (!moveTowardInteractable(target_entity, interaction_range_sq)) {
				_player->stop();
				_target_interactable = nullptr;
			}
			return true;
		}

		_player->stop();
		_target_interactable->onInteract(*_player);

		if (const auto cat = std::dynamic_pointer_cast<Entity::Cat>(_target_interactable)) {
			_engine->getUIHandler().openDialogue(cat->getDialogueTree());
			_engine->getQuestManager().notifyNPCTalked(cat->getName());
			_target_interactable = nullptr;
			return true;
		}

		const auto clickable = std::dynamic_pointer_cast<Entity::InteractiveClickable>(_target_interactable);
		if (clickable && clickable->getInventory() != nullptr)
			_engine->getUIHandler().openContainer(clickable.get());

		_target_interactable = nullptr;
		return true;
	}

	void PlayerController::useAbility(const int index, const float target_x, const float target_y) const {
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

	void PlayerController::handleMouseInput(Vector3 mouse_world_pos, const float screen_x, const float screen_y) {
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

			if (trySelectEnemy(screen_x, screen_y))
				return;
		}

		_target_interactable = nullptr;
		handleGroundClick(mouse_world_pos);
	}

	void PlayerController::handleKeyboardInput(Vector3 mouse_world_pos, const float screen_x, const float screen_y) {
		if (_engine->getUIHandler().isInputBlocked()) return;

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

	void PlayerController::processPendingAction() {
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
		if (_player->isAnimationLocked()) return;

		const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(_target_enemy);
		if (!isValidEnemyTarget(enemy)) {
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

	bool PlayerController::trySelectEnemy(const float screen_x, const float screen_y) {
		const auto entity = _engine->getEntityAt(screen_x, screen_y);
		const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(entity);
		if (!isValidEnemyTarget(enemy))
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
		if (!enemy || isValidEnemyTarget(enemy))
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
					if (const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(target); isValidEnemyTarget(enemy)) {
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
			if (isValidEnemyTarget(enemy)) {
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
		_target_enemy = nullptr;
	}

	void PlayerController::updatePathMovement() {
		if (!_player->isMoving() && !_current_path.empty()) {
			_current_path.erase(_current_path.begin());

			if (!_current_path.empty())
				_player->moveTo(_current_path.front().x, _current_path.front().y);
		} else if (!_current_path.empty() && _target_enemy) {
			_current_path.clear();
		}
	}

	bool PlayerController::moveTowardInteractable(const std::shared_ptr<Entity::Entity>& target, const float interaction_range_sq) {
		if (!target)
			return false;

		const float interaction_range = std::sqrt(std::max(0.0f, interaction_range_sq));
		const float approach_radius = std::max(0.5f, interaction_range * 0.75f);
		const Vector2 target_center = target->getCenter();
		const Vector3 target_world = target->getWorldPos3D();

		if (buildPathToWorldPosition(target_world)) {
			moveAlongCurrentPath();
			return true;
		}

		constexpr int candidate_count = 12;
		for (int i = 0; i < candidate_count; ++i) {
			const float angle = (static_cast<float>(i) / candidate_count) * 2.0f * PI;
			const Vector3 candidate = {
				target_center.x + std::cos(angle) * approach_radius,
				target_world.y,
				target_center.y + std::sin(angle) * approach_radius
			};

			if (buildPathToWorldPosition(candidate)) {
				moveAlongCurrentPath();
				return true;
			}
		}

		return false;
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
		if (_target_enemy)
			_player->rotateTowardsCenter(_target_enemy->getCenter().x, _target_enemy->getCenter().y);
		else if (!_player->isMoving() && !_player->isAnimationLocked())
			_player->rotateTowards(_last_mouse_x, _last_mouse_y);
	}

} // namespace Nawia::Core
