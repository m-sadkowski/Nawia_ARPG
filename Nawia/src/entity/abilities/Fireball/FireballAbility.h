#pragma once
#include "Ability.h"

namespace Nawia::Entity
{

    class FireballAbility : public Ability {
    public:
        FireballAbility(const std::shared_ptr<SDL_Texture>& projectile_tex);

        std::unique_ptr<Entity> cast(Entity* caster, float target_x, float target_y) override;

    private:
        std::shared_ptr<SDL_Texture> _texture;
    };
	
} // namespace Nawia::Entity

