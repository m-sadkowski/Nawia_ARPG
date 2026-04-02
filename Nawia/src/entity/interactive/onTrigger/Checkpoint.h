#pragma once
#include "Interactable.h"
#include "Entity.h"
#include "InteractiveTrigger.h"

namespace Nawia::Entity {

    class Checkpoint : public InteractiveTrigger {
    public:
        Checkpoint(const std::string& name, float x, float y);

        void onTriggerEnter(Entity& other) override;
        void update(float delta_time) override;
        void render(const Camera3D& camera) override;

        [[nodiscard]] bool isActivated() const { return _activated; }
        float getInteractionRange() override;
    private:
        bool _activated = false;
    };

} // namespace Nawia::Entity
