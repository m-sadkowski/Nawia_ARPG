#pragma once
#include "Spell.h"

namespace Nawia::Core
{

    class FireballSpell : public Spell {
    public:
        FireballSpell(const std::shared_ptr<SDL_Texture>& projectile_tex);

        std::unique_ptr<Entity::Entity> cast(Entity::Entity* caster, float target_x, float target_y) override;

    private:
        std::shared_ptr<SDL_Texture> _texture;
    };
	
} // namespace Nawia::Core

