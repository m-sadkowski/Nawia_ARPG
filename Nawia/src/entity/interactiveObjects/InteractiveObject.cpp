#include "InteractiveObject.h"
#include <cmath>

namespace Nawia::Entity {

    InteractiveObject::InteractiveObject(float start_x, float start_y,
        const std::shared_ptr<SDL_Texture>& texture, int max_hp)
        : Entity(start_x, start_y, texture, max_hp)
    {
    }

    float InteractiveObject::getInteractionRange() const {
        return 1.2f;
    }
    // move to usable
    /*bool InteractiveObject::isPlayerInRange(float player_x, float player_y) const {
        
        float dx = _pos->getX() - player_x;
        float dy = _pos->getY() - player_y;
        float distance = std::sqrt(dx * dx + dy * dy);

        return distance <= getInteractionRange();
    }*/

} // namespace Nawia::Entity