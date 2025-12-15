#pragma once
#include "Entity.h"
#include "SpellEffect.h"
#include <string>
#include <vector>
#include <functional>

namespace Nawia::Core {

    class Engine;

    enum class SpellTargetType {
        POINT,  // casted in location
        UNIT,   // casted on enemies
        SELF    // casted on player
    };

    class Spell {
    public:
        Spell(std::string name, float cooldown, float cast_range);

        virtual ~Spell() = default;

        void update(float dt);
        bool isReady() const;
        // returns entity that engine needs to spawn with casted spell
        virtual std::unique_ptr<Entity::Entity> cast(Entity::Entity* caster, float target_x, float target_y) = 0;
        std::string getName() const;

    protected:
        std::string _name;
        float _cooldown_max;
        float _cooldown_timer;
        float _cast_range;

        void startCooldown() { _cooldown_timer = _cooldown_max; }
    };
}