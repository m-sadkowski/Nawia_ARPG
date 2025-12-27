
#include "PlayerController.h"
#include "Engine.h"
#include "Logger.h"

#include <EnemyInterface.h>
#include <string>

namespace Nawia::Core {

	PlayerController::PlayerController(Engine* engine, std::shared_ptr<Entity::Player> player) : _engine(engine), _player(std::move(player)) {}

	void PlayerController::handleInput(const float mouse_world_x, const float mouse_world_y, const float screen_x, const float screen_y) 
	{
		// handle input (queue or immediate)
		const bool is_locked = _player->isAnimationLocked();

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) 
		{
			// check for entity clicks first
			if (const auto entity = _engine->getEntityAt(screen_x, screen_y)) 
			{
				if (const auto enemy =  std::dynamic_pointer_cast<Entity::EnemyInterface>(entity)) 
				{
					// check target validity: alive and belongs to an enemy faction
					if (!enemy->isDead() && enemy->getFaction() != Entity::Entity::Faction::None)
					{
						if (is_locked)
						{
							_pending_action = { PendingAction::Type::Move, 0.0f, 0.0f, -1, enemy };
						}
						else
						{
							_target_enemy = enemy;
							_pending_action = {}; // clear pending on new valid action
						}
						return;
					}
					// if the enemy is dead or has no faction (dying), we fall through 
					// to process this as a ground click (movement), avoiding "eaten clicks"
				}
			}

			// ground click
			if (is_locked)
			{
				_pending_action = { PendingAction::Type::Move, mouse_world_x, mouse_world_y, -1,  std::weak_ptr<Entity::Entity>() };
			}
			else
			{
				_target_enemy = nullptr;
				_player->moveTo(mouse_world_x, mouse_world_y);
				_pending_action = {}; // clear pending on new valid action
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
				
				// try to find target for Unit-target abilities even when queuing
				if (const auto ability = _player->getAbility(ability_index))
				{
					if (ability->getTargetType() == Entity::AbilityTargetType::UNIT)
					{
						if (const auto target = _engine->getEntityAt(screen_x, screen_y))
						{
							if (const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(target))
							{
								// if invalid target, allow standard valid target check (or just ignore specific unit lock)
								if (enemy->isDead() || enemy->getFaction() == Entity::Entity::Faction::None)
								{
									// do not lock onto this target
								}
								else
								{
									_pending_action.target = target;
								}
							}
							else
							{
								_pending_action.target = target;
							}
							// logic below handles "useAbility" which checks validity
						}
					}
				}
			}
			else
			{
				if (const auto ability = _player->getAbility(ability_index)) 
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
										useAbility(ability_index, enemy->getCenter().x, enemy->getCenter().y);
								}
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
		// execute pending actions if unlocked
		if (!_player->isAnimationLocked() && _pending_action.type != PendingAction::Type::None)
		{
			if (_pending_action.type == PendingAction::Type::Move)
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
			else if (_pending_action.type == PendingAction::Type::Ability)
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

			_pending_action = {}; // reset
		}

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
        
		// defines a small buffer zone to prevent the character from jittering at the edge of attack range
		constexpr float hysteresis = 0.5f;

		if (dist_sq > (attack_cast_range + hysteresis) * (attack_cast_range + hysteresis)) 
		{
			_player->moveTo(_target_enemy->getX(), _target_enemy->getY());
		} 
		else if (dist_sq <= attack_cast_range * attack_cast_range) 
		{
			_player->rotateTowards(_target_enemy->getCenter().x, _target_enemy->getCenter().y);
			_player->moveTo(_player->getX(), _player->getY());
			useAbility(auto_attack_index, _target_enemy->getCenter().x, _target_enemy->getCenter().y);
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
			_player->rotateTowards(_target_enemy->getCenter().x, _target_enemy->getCenter().y);
			_player->moveTo(_player->getX(), _player->getY());
			useAbility(auto_attack_index, _target_enemy->getCenter().x, _target_enemy->getCenter().y);
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
