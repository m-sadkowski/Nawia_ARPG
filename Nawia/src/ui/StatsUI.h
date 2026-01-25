#pragma once
#include <raylib.h>
#include <memory>

namespace Nawia::Entity { class Player; }

namespace Nawia::UI {
    class StatsUI {
    public:
        StatsUI(std::shared_ptr<Entity::Player> player);
        void render(float x, float y) const;
    private:
        std::shared_ptr<Entity::Player> _player;
    };
}
