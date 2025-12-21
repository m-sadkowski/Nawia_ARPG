#pragma once
#include "Entity.h"

namespace Nawia::Entity {

    class InteractiveObject : public Entity {
    public:

        InteractiveObject(float start_x, float start_y,
            const std::shared_ptr<SDL_Texture>& texture, int max_hp);

        [[nodiscard]] virtual bool isInteractive() const = 0;
    	virtual void interaction() = 0;

       
        [[nodiscard]] virtual float getInteractionRange() const;

      
        // move to Usable [[nodiscard]] bool isPlayerInRange(float player_x, float player_y) const;

    };

} // namespace Nawia::Entity