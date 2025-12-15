#include "PlayerController.h"
#include "Enemy.h"
#include "Engine.h"
#include "Logger.h"

namespace Nawia::Core 
{

	PlayerController::PlayerController(Engine *engine,
	                                   std::shared_ptr<Entity::Player> player)
	    : _engine(engine), _player(std::move(player)) {}

	void PlayerController::handleInput(const SDL_Event &event, const float mouse_world_x, const float mouse_world_y, const float screen_x, const float screen_y) const {
	  if (event.type == SDL_EVENT_KEY_DOWN) {
	    int spellIndex = -1;
	    if (event.key.key == SDLK_Q)
	      spellIndex = 0;
	    else if (event.key.key == SDLK_W)
	      spellIndex = 1;
	    else if (event.key.key == SDLK_E)
	      spellIndex = 2;
	    else if (event.key.key == SDLK_R)
	      spellIndex = 3;

	    if (spellIndex == -1)
	        return;

  		if (const auto spell = _player->getSpell(spellIndex)) {
  			switch (spell->getTargetType())
  			{
  			case SpellTargetType::UNIT:
  				if (const auto target = _engine->getEntityAt(screen_x, screen_y))
  					if (const auto enemy = std::dynamic_pointer_cast<Entity::Enemy>(target))
  						castSpell(spellIndex, enemy->getX(), enemy->getY());
  				break;
  			case SpellTargetType::POINT:
	            castSpell(spellIndex, mouse_world_x, mouse_world_y);
  				break;
	        case SpellTargetType::SELF:

	            break;
  			default:
  				break;
  			}
	    }
	  }
	}

	void PlayerController::castSpell(const int index, const float target_x,
	                                 const float target_y) const {
	  if (const auto spell = _player->getSpell(index))
	    if (spell->isReady())
	      if (auto effect = spell->cast(_player.get(), target_x, target_y)) {
	        Logger::debugLog("Spell " + std::to_string(index) + " casted, target location: (" + std::to_string(target_x) + ", " + std::to_string(target_y) + ")");
	        _engine->spawnEntity(std::move(effect));
	      }
	}

} // namespace Nawia::Core
