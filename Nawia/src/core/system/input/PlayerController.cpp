#include "PlayerController.h"
#include "Engine.h"
#include "Logger.h"

#include <Enemy.h>

namespace Nawia::Core 
{

	PlayerController::PlayerController(Engine *engine, std::shared_ptr<Entity::Player> player) : _engine(engine), _player(std::move(player)) {}

	void PlayerController::handleInput(const SDL_Event &event, const float mouse_world_x, const float mouse_world_y, const float screen_x, const float screen_y) const {
	  if (event.type == SDL_EVENT_KEY_DOWN) {
	    int abilityIndex = -1;
	    if (event.key.key == SDLK_Q)
			abilityIndex = 0;
	    else if (event.key.key == SDLK_W)
			abilityIndex = 1;
	    else if (event.key.key == SDLK_E)
			abilityIndex = 2;
	    else if (event.key.key == SDLK_R)
			abilityIndex = 3;

	    if (abilityIndex == -1)
	        return;

  		if (const auto ability = _player->getAbility(abilityIndex)) {
  			switch (ability->getTargetType())
  			{
  			case Entity::AbilityTargetType::UNIT:
  				if (const auto target = _engine->getEntityAt(screen_x, screen_y))
  					if (const auto enemy = std::dynamic_pointer_cast<Entity::Enemy>(target))
  						useAbility(abilityIndex, enemy->getX(), enemy->getY());
  				break;
  			case Entity::AbilityTargetType::POINT:
				useAbility(abilityIndex, mouse_world_x, mouse_world_y);
  				break;
	        case Entity::AbilityTargetType::SELF:
  			default:
  				break;
  			}
	    }
	  }
	}

	void PlayerController::useAbility(const int index, const float target_x, const float target_y) const {
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
