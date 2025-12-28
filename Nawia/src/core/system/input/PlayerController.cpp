
#include "PlayerController.h"
#include "Engine.h"
#include "Logger.h"

#include <EnemyInterface.h>
#include <string>

namespace Nawia::Core {

	PlayerController::PlayerController(Engine* engine, std::shared_ptr<Entity::Player> player) : _engine(engine), _player(std::move(player)) {}

	void PlayerController::handleInput(const float mouse_world_x, const float mouse_world_y, const float screen_x, const float screen_y) 
	{
		handleMouseInput(mouse_world_x, mouse_world_y, screen_x, screen_y);
		handleKeyboardInput(mouse_world_x, mouse_world_y, screen_x, screen_y);
	}

	void PlayerController::update(float dt) 
	{
		processPendingAction();
		processAutoAttack();
	}

	void PlayerController::useAbility(const int index, const float target_x, const float target_y) const 
	{
		const auto spell = _player->getAbility(index);

		if (!spell || !spell->isReady())
			return;

		if (auto effect = spell->cast(target_x, target_y)) 
		{
			Logger::debugLog("Ability " + std::to_string(index) + " used, target location: (" + std::to_string(target_x) + ", " + std::to_string(target_y) + ")");
			_engine->spawnEntity(std::move(effect));
		}
	}

	void PlayerController::handleMouseInput(const float mouse_world_x, const float mouse_world_y, const float screen_x, const float screen_y)
	{
		if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

		// check for entity clicks first
		if (trySelectEnemy(screen_x, screen_y))
			return;

		// ground click
		handleGroundClick(mouse_world_x, mouse_world_y);
	}

	void PlayerController::handleKeyboardInput(const float mouse_world_x, const float mouse_world_y, const float screen_x, const float screen_y)
	{
		int ability_index = -1;

		if (IsKeyPressed(KEY_Q)) ability_index = 0;
		if (IsKeyPressed(KEY_W)) ability_index = 1;
		if (IsKeyPressed(KEY_E)) ability_index = 2;
		if (IsKeyPressed(KEY_R)) ability_index = 3;

		if (ability_index == -1) return;

		const bool is_locked = _player->isAnimationLocked();

		if (is_locked)
		{
			queueAbility(ability_index, mouse_world_x, mouse_world_y, screen_x, screen_y);
		}
		else
		{
			castAbility(ability_index, mouse_world_x, mouse_world_y, screen_x, screen_y);
		}
	}

	void PlayerController::processPendingAction()
	{
		// execute pending actions if unlocked
		if (_player->isAnimationLocked() || _pending_action.type == PendingAction::Type::None)
			return;

		if (_pending_action.type == PendingAction::Type::Move)
		{
			processPendingMove();
		}
		else if (_pending_action.type == PendingAction::Type::Ability)
		{
			processPendingAbility();
		}

		_pending_action = {}; // reset
	}

	void PlayerController::processAutoAttack()
	{
		if (!_target_enemy || _target_enemy->isDead() || _target_enemy->getFaction() == Entity::Entity::Faction::None)
		{
			_target_enemy = nullptr;
			return;
		}

		const float dx = _target_enemy->getX() - _player->getX();
		const float dy = _target_enemy->getY() - _player->getY();
		const float dist_sq = dx * dx + dy * dy;

		constexpr int auto_attack_index = 0;
		const float attack_cast_range =  _player->getAbility(auto_attack_index)->getCastRange();

		updateCombatMovement(dist_sq, attack_cast_range);
	}

	bool PlayerController::trySelectEnemy(const float screen_x, const float screen_y)
	{
		const auto entity = _engine->getEntityAt(screen_x, screen_y);
		if (!entity) return false;

		const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(entity);
		if (!enemy) return false;

		// check target validity: alive and belongs to an enemy faction
		if (enemy->isDead() || enemy->getFaction() == Entity::Entity::Faction::None)
			return false; // treat as ground click

		if (_player->isAnimationLocked())
		{
			_pending_action = { PendingAction::Type::Move, 0.0f, 0.0f, -1, enemy };
		}
		else
		{
			_target_enemy = enemy;
			_pending_action = {}; // clear pending on new valid action
		}
		return true;
	}

	void PlayerController::handleGroundClick(const float x, const float y)
	{
		if (_player->isAnimationLocked())
		{
			_pending_action = { PendingAction::Type::Move, x, y, -1,  std::weak_ptr<Entity::Entity>() };
		}
		else
		{
			_target_enemy = nullptr;
			_player->moveTo(x, y);
			_pending_action = {}; // clear pending on new valid action
		}
	}

	void PlayerController::queueAbility(const int index, const float x, const float y, const float screen_x, const float screen_y)
	{
		_pending_action = { PendingAction::Type::Ability, x, y, index, std::weak_ptr<Entity::Entity>() };
		
		// try to find target for Unit-target abilities even when queuing
		if (const auto ability = _player->getAbility(index))
		{
			if (ability->getTargetType() == Entity::AbilityTargetType::UNIT)
			{
				if (const auto target = _engine->getEntityAt(screen_x, screen_y))
				{
					// if invalid target, allow standard valid target check (or just ignore specific unit lock)
					if (const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(target))
					{
						if (!enemy->isDead() && enemy->getFaction() != Entity::Entity::Faction::None)
						{
							_pending_action.target = target;
						}
					}
					else
					{
						_pending_action.target = target;
					}
				}
			}
		}
	}

	void PlayerController::castAbility(const int index, const float x, const float y, const float screen_x, const float screen_y)
	{
		if (const auto ability = _player->getAbility(index)) 
		{
			_pending_action = {}; // clear pending on new valid action
			switch (ability->getTargetType()) 
			{
				case Entity::AbilityTargetType::UNIT:
					if (const auto target = _engine->getEntityAt(screen_x, screen_y))
						if (const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(target))
						{
							// only cast if valid target
							if (!enemy->isDead() && enemy->getFaction() != Entity::Entity::Faction::None)
								useAbility(index, enemy->getCenter().x, enemy->getCenter().y);
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
	}

	void PlayerController::processPendingMove()
	{
		_target_enemy = nullptr; // default clear
		
		if (const auto target = _pending_action.target.lock())
		{
			// if we queued a move on an enemy, set as target
			if (const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(target))
			{
				_target_enemy = enemy;
			}
		}
		else
		{
			// ground move
			_player->moveTo(_pending_action.x, _pending_action.y);
		}
	}

	void PlayerController::processPendingAbility()
	{
		// simplify ability execution
		if (const auto ability = _player->getAbility(_pending_action.ability_index))
		{
			if (ability->getTargetType() == Entity::AbilityTargetType::UNIT)
			{
				if (const auto target = _pending_action.target.lock())
				{
					if (const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(target))
						useAbility(_pending_action.ability_index, enemy->getCenter().x, enemy->getCenter().y);
				}
			}
			else
			{
				useAbility(_pending_action.ability_index, _pending_action.x, _pending_action.y);
			}
		}
	}

	void PlayerController::updateCombatMovement(const float dist_sq, const float attack_range)
	{
		// defines a small buffer zone to prevent the character from jittering at the edge of attack range
		constexpr float hysteresis = 0.5f;
		constexpr int auto_attack_index = 0;

		const float hit_range = attack_range + hysteresis;

		if (dist_sq > hit_range * hit_range) 
		{
			_player->moveTo(_target_enemy->getX(), _target_enemy->getY());
		} 
		else 
		{
			// In range (or within hysteresis buffer)
			_player->rotateTowards(_target_enemy->getCenter().x, _target_enemy->getCenter().y);
			_player->moveTo(_player->getX(), _player->getY()); // Stop moving
			useAbility(auto_attack_index, _target_enemy->getCenter().x, _target_enemy->getCenter().y);
		}
	}

} // namespace Nawia::Core
