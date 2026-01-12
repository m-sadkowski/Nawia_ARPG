#pragma once
#include "Interactable.h"
#include "Entity.h"
#include "InteractiveTrigger.h"

namespace Nawia::Entity {

    class Checkpoint : public InteractiveTrigger {
    public:
        Checkpoint(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture);

        // Implementujemy tylko to, co nas interesuje dla triggera
        void onTriggerEnter(Entity& other) override;

        void update(float delta_time) override;
        void render(float offset_x, float offset_y) override;

        [[nodiscard]] bool isActivated() const { return _activated; }
        float getInteractionRange() override;
    private:
        bool _activated = false;
    };

} // namespace Nawia::Entity#pragma once
