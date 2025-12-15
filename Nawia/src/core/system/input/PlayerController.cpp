#include "PlayerController.h"
#include "Engine.h"
#include "Logger.h"

namespace Nawia::Core {

    PlayerController::PlayerController(Engine* engine, std::shared_ptr<Entity::Player> player) : _engine(engine), _player(std::move(player)) {}

    void PlayerController::handleInput(const SDL_Event& event, const float mouse_world_x, const float mouse_world_y) const {
        if (event.type == SDL_EVENT_KEY_DOWN) {
            int spellIndex = -1;
            if (event.key.key == SDLK_Q) spellIndex = 0;
            else if (event.key.key == SDLK_W) spellIndex = 1;
            else if (event.key.key == SDLK_E) spellIndex = 2;
            else if (event.key.key == SDLK_R) spellIndex = 3;

            if (spellIndex != -1) {
                castSpell(spellIndex, mouse_world_x, mouse_world_y);
            }
        }
    }

    void PlayerController::castSpell(const int index, const float target_x, const float target_y) const {
        if (const auto spell = _player->getSpell(index))
            if (spell->isReady())
				if (auto effect = spell->cast(_player.get(), target_x, target_y))
				{
                    Logger::debugLog("Spell " + std::to_string(index) + " casted, target location: (" + std::to_string(target_x) + ", " + std::to_string(target_y) + ")");
                    _engine->spawnEntity(std::move(effect));
				}
    }

} // namespace Nawia::Core
