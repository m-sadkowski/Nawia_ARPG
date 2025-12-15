#include "FireballSpell.h"
#include "Projectile.h"

namespace Nawia::Core
{
    FireballSpell::FireballSpell(const std::shared_ptr<SDL_Texture>& projectile_tex)
        : Spell("Fireball", 1.0f, 100.0f), _texture(projectile_tex) {}

    std::unique_ptr<Entity::Entity> FireballSpell::cast(Entity::Entity* caster, float target_x, float target_y) {
        if (!isReady()) 
            return nullptr;

        startCooldown();

        constexpr float speed = 10.0f;
        constexpr int damage = 20;
        float duration = _cast_range / speed;

        return std::make_unique<Entity::Projectile>(caster->getX(), caster->getY(),target_x, target_y, speed, _texture, damage, duration);
    }
}
