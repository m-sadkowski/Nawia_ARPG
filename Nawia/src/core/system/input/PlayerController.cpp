
#include "PlayerController.h"
#include "Engine.h"
#include "Logger.h"

#include <EnemyInterface.h>
#include <string>

namespace Nawia::Core {

	PlayerController::PlayerController(Engine* engine, std::shared_ptr<Entity::Player> player) : _engine(engine), _player(std::move(player)) {}

	void PlayerController::handleInput(const float mouse_world_x, const float mouse_world_y, const float screen_x, const float screen_y) 
	{
		// 1. Handle Input (Queue or Immediate)
		bool is_locked = _player->isAnimationLocked();

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) 
		{
			// Check for entity clicks first
			if (const auto entity = _engine->getEntityAt(screen_x, screen_y)) 
			{
				if (const auto enemy =  std::dynamic_pointer_cast<Entity::EnemyInterface>(entity)) 
				{
					if (is_locked)
					{
						_pending_action = { PendingAction::Type::Move, 0.0f, 0.0f, -1, enemy };
					}
					else
					{
						_target_enemy = enemy;
						_pending_action = {}; // Clear pending on new valid action
					}
					return;
				}
			}

			// Ground Click
			if (is_locked)
			{
				_pending_action = { PendingAction::Type::Move, mouse_world_x, mouse_world_y, -1,  std::weak_ptr<Entity::Entity>() };
			}
			else
			{
				_target_enemy = nullptr;
				_player->moveTo(mouse_world_x, mouse_world_y);
				_pending_action = {}; // Clear pending on new valid action
			}
		}

		int ability_index = -1;

		if (IsKeyPressed(KEY_Q)) ability_index = 0;
		if (IsKeyPressed(KEY_W)) ability_index = 1;
		if (IsKeyPressed(KEY_E)) ability_index = 2;
		if (IsKeyPressed(KEY_R)) ability_index = 3;

		if (ability_index != -1)
		{
			if (is_locked)
			{
				_pending_action = { PendingAction::Type::Ability, mouse_world_x, mouse_world_y, ability_index, std::weak_ptr<Entity::Entity>() };
				
				// Optional: Try to find target for Unit-target abilities even when queuing
				if (const auto ability = _player->getAbility(ability_index))
				{
					if (ability->getTargetType() == Entity::AbilityTargetType::UNIT)
					{
						if (const auto target = _engine->getEntityAt(screen_x, screen_y))
						{
							_pending_action.target = target;
							// If unit target ability but no target -> invalid queue? 
							// Logic below handles "useAbility" which checks validity
						}
					}
				}
			}
			else
			{
				if (const auto ability = _player->getAbility(ability_index)) 
				{
					_pending_action = {}; // Clear pending on new valid action
					switch (ability->getTargetType()) 
					{
						case Entity::AbilityTargetType::UNIT:
							if (const auto target = _engine->getEntityAt(screen_x, screen_y))
								if (const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(target))
									useAbility(ability_index, enemy->getX(), enemy->getY());
						break;

						case Entity::AbilityTargetType::POINT:
							useAbility(ability_index, mouse_world_x, mouse_world_y);
							break;

						case Entity::AbilityTargetType::SELF:
						default:
							break;
					}
				}
			}
		}
	}

	void PlayerController::update(float dt) 
	{
		// 1. Execute Pending Actions if Unlocked
		if (!_player->isAnimationLocked() && _pending_action.type != PendingAction::Type::None)
		{
			if (_pending_action.type == PendingAction::Type::Move)
			{
				_target_enemy = nullptr; // Default clear
				
				if (auto target = _pending_action.target.lock())
				{
					// If we queued a move on an enemy, set as target
					if (auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(target))
					{
						_target_enemy = enemy;
					}
				}
				else
				{
					// Ground move
					_player->moveTo(_pending_action.x, _pending_action.y);
				}
			}
			else if (_pending_action.type == PendingAction::Type::Ability)
			{
				// Simplify ability execution
				if (const auto ability = _player->getAbility(_pending_action.ability_index))
				{
					if (ability->getTargetType() == Entity::AbilityTargetType::UNIT)
					{
						if (auto target = _pending_action.target.lock())
						{
							if (auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(target))
								useAbility(_pending_action.ability_index, enemy->getX(), enemy->getY());
						}
					}
					else
					{
						useAbility(_pending_action.ability_index, _pending_action.x, _pending_action.y);
					}
				}
			}

			_pending_action = {}; // Reset
		}

		if (!_target_enemy || _target_enemy->isDead())
		{
			_target_enemy = nullptr;
			return;
		}

		const float dx = _target_enemy->getX() - _player->getX();
		const float dy = _target_enemy->getY() - _player->getY();
		const float dist_sq = dx * dx + dy * dy;

		constexpr int auto_attack_index = 0;
		const float attack_cast_range =  _player->getAbility(auto_attack_index)->getCastRange();
        
		// defines a small buffer zone to prevent the character from jittering at the edge of attack range
		constexpr float hysteresis = 0.5f;

		if (dist_sq > (attack_cast_range + hysteresis) * (attack_cast_range + hysteresis)) 
		{
			_player->moveTo(_target_enemy->getX(), _target_enemy->getY());
		} 
		else if (dist_sq <= attack_cast_range * attack_cast_range) 
		{
			_player->moveTo(_player->getX(), _player->getY());
			useAbility(auto_attack_index, _target_enemy->getX(), _target_enemy->getY());
		}

		if (dist_sq > attack_cast_range * attack_cast_range) 
		{
			constexpr float tolerance = 0.2f;
			if (dist_sq > (attack_cast_range + tolerance) * (attack_cast_range + tolerance)) 
			{
			  _player->moveTo(_target_enemy->getX(), _target_enemy->getY());
			}
		} 
		else 
		{
			_player->moveTo(_player->getX(), _player->getY());
			useAbility(auto_attack_index, _target_enemy->getX(), _target_enemy->getY());
		}
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

} // namespace Nawia::Core
