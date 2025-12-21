#include "PlayerController.h"
#include "Engine.h"
#include "Logger.h"

#include <EnemyInterface.h>

namespace Nawia::Core {

	PlayerController::PlayerController(Engine *engine, std::shared_ptr<Entity::Player> player) : _engine(engine), _player(std::move(player)) {}

	void PlayerController::handleInput(const SDL_Event &event, const float mouse_world_x, const float mouse_world_y, const float screen_x, const float screen_y)
	{
		if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) 
		{
		    if (const auto entity = _engine->getEntityAt(screen_x, screen_y)) 
			{
    			if (const auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(entity))
				{
					_target_enemy = enemy;
					return;
				}
		    }

		    _target_enemy = nullptr;
		    _player->moveTo(mouse_world_x, mouse_world_y);
		}

		if (event.type == SDL_EVENT_KEY_DOWN) 
		{
	  		int ability_index;
			switch (event.key.key)
			{
			case SDLK_Q:
				ability_index = 0;
				break;
			case SDLK_W:
				ability_index = 1;
				break;
			case SDLK_E:
				ability_index = 2;
				break;
			case SDLK_R:
				ability_index = 3;
				break;
			default:
				return;
			}

		    if (const auto ability = _player->getAbility(ability_index)) {
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

	void PlayerController::update(float dt) 
	{
		if (!_target_enemy)
			return;

		if (_target_enemy->isDead()) {
			_target_enemy = nullptr;
			return;
		}

		const float dx = _target_enemy->getX() - _player->getX();
		const float dy = _target_enemy->getY() - _player->getY();
		const float dist_sq = dx * dx + dy * dy;
		
		constexpr int auto_attack_index = 0;
		const float attack_cast_range = _player->getAbility(auto_attack_index)->getCastRange();

		if (dist_sq > attack_cast_range * attack_cast_range)
		{
			_player->moveTo(_target_enemy->getX(), _target_enemy->getY());
		} 
		else 
		{
			// we stop the player by moving him to his current location
			_player->moveTo(_player->getX(), _player->getY());
			useAbility(auto_attack_index, _target_enemy->getX(), _target_enemy->getY());
		}
	}

	void PlayerController::useAbility(const int index, const float target_x, const float target_y) const 
	{
		if (const auto spell = _player->getAbility(index)) 
		{
			if (!spell->isReady())
			return;

			if (auto effect = spell->cast(_player.get(), target_x, target_y)) {
				Logger::debugLog("Ability " + std::to_string(index) + " used, target location: (" + std::to_string(target_x) + ", " + std::to_string(target_y) + ")");
				_engine->spawnEntity(std::move(effect));
			}
		}
	}

} // namespace Nawia::Core
