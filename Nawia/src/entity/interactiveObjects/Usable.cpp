#include "Usable.h"


namespace Nawia::Entity
{
    bool Usable::isPlayerInRange(float player_x, float player_y) const {

        const float dx = _pos->getX() - player_x;
        const float dy = _pos->getY() - player_y;
        const float distance = std::sqrt(dx * dx + dy * dy);

        return distance <= getInteractionRange();
    }
}
