#pragma once
#include "InteractiveTrigger.h"

namespace Nawia::Entity {

    class BossArenaTrigger : public InteractiveTrigger {
    public:
        BossArenaTrigger(const std::string& boss_id, float x, float y, float width, float height);

        void onTriggerEnter(Entity& other) override;
        void update(float dt) override;
        void render(const Camera3D& camera) override;

    private:
        std::string _boss_id;
        bool _activated = false;
    };

} // namespace Nawia::Entity
