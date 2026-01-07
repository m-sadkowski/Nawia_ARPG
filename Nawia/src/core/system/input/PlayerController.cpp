
#include "PlayerController.h"
#include "Engine.h"
#include "Logger.h"
#
#include <EnemyInterface.h>
#include <string>



namespace Nawia::Core {

	PlayerController::PlayerController(Engine* engine, std::shared_ptr<Entity::Player> player) : _engine(engine), _player(std::move(player)) {}

	void PlayerController::handleInput(const float mouse_world_x, const float mouse_world_y, const float screen_x, const float screen_y) 
	{
		if (!_player) return;

		_last_mouse_x = mouse_world_x;
		_last_mouse_y = mouse_world_y;

		handleMouseInput(mouse_world_x, mouse_world_y, screen_x, screen_y);
		handleKeyboardInput(mouse_world_x, mouse_world_y, screen_x, screen_y);
	}

	void PlayerController::update(const float dt) 
	{
		processPendingAction();
		if (_target_interactable) {
			auto target_ent = std::dynamic_pointer_cast<Entity::Entity>(_target_interactable);
			if (target_ent) {
				const float dx = target_ent->getX() - _player->getX();
				const float dy = target_ent->getY() - _player->getY();
				const float dist_sq = dx * dx + dy * dy;
				constexpr float interact_range_sq = 2.5f * 2.5f;

				if (dist_sq > interact_range_sq) {
					_player->moveTo(target_ent->getX(), target_ent->getY());
				}
				else {
					_player->stop();
					_target_interactable->onInteract(*_player);
					_target_interactable = nullptr;
				}
				return; 
			}
		}
		processAutoAttack();

		if (_target_enemy && _target_enemy->isDead()) 
		{
			_target_enemy = nullptr;
		}

		
		if (_target_enemy)
		{
			
			_player->rotateTowardsCenter(_target_enemy->getCenter().x, _target_enemy->getCenter().y);
		}
		else if (!_player->isMoving() && !_player->isAnimationLocked())
		{
			)
			_player->rotateTowards(_last_mouse_x, _last_mouse_y);
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

	void PlayerController::handleMouseInput(const float mouse_world_x, const float mouse_world_y, const float screen_x, const float screen_y)
	{
		if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

		const auto entity = _engine->getEntityAt(screen_x, screen_y);

		if (entity)
		{
			

			
			auto interactable = std::dynamic_pointer_cast<Entity::Interactable>(entity);

			if (interactable)
			{
				

				if (interactable->canInteract())
				{
					if (_player->isAnimationLocked())
					{
						
						_pending_action = { PendingAction::Type::Interact, 0.0f, 0.0f, -1, entity };
					}
					else
					{
						
						_target_interactable = interactable;
						_target_enemy = nullptr;
						_pending_action = {};
					}
					return;
				}
				
			}
			

			// 2. Jeœli nie interakcja, sprawdzamy walkê
			if (trySelectEnemy(screen_x, screen_y)) {
				
				return;
			}
		}
		else
		{
			
		}

		_target_interactable = nullptr;
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

		const Vector2 player_center = _player->getCenter();
		const Vector2 enemy_center = _target_enemy->getCenter();

		const float dx = enemy_center.x - player_center.x;
		const float dy = enemy_center.y - player_center.y;
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
			// moveTo already corrects for offset, so we pass the exact target point we want to reach (or get close to)
			// passing center ensures we move towards the center of the enemy
			_player->moveTo(_target_enemy->getCenter().x, _target_enemy->getCenter().y);
		} 
		else 
		{
			// In range (or within hysteresis buffer)
			_player->rotateTowards(_target_enemy->getCenter().x, _target_enemy->getCenter().y);
			_player->stop(); // Stop moving cleanly
			useAbility(auto_attack_index, _target_enemy->getCenter().x, _target_enemy->getCenter().y);
		}
	}

} // namespace Nawia::Core
