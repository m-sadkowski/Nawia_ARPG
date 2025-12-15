#pragma once
#include "Entity.h"

namespace Nawia::Entity {

    class SpellEffect : public Entity {
    public:
        SpellEffect(float x, float y, const std::shared_ptr<SDL_Texture>& tex, float duration, int damage);

        void update(float dt) override;
        bool isExpired() const;
        int getDamage() const;

    protected:
        float _duration;
        float _timer;
        int _damage;
    };

} // namespace Nawia::Entity