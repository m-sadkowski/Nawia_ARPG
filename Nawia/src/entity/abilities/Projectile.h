#pragma once
#include "AbilityEffect.h"

namespace Nawia::Entity
{

    class Projectile : public AbilityEffect {
    public:
        Projectile(float x, float y, float target_x, float target_y, float speed, const std::shared_ptr<SDL_Texture>& tex, int damage, float duration);

        void update(float dt) override;

    private:
        float _speed;
        float _vel_x, _vel_y;
    };

} // namespace Nawia::Entity