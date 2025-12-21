#pragma once
#include "Trigger.h"

namespace Nawia::Entity {

    class Checkpoint : public Trigger {
    public:
        Checkpoint(float x, float y, const std::shared_ptr<SDL_Texture>& texture);

        void update(float delta_time) override;
        [[nodiscard]] bool isInteractive() const override;
        void interaction() override;
        [[nodiscard]] float getInteractionRange() const override { return 0.5f; }

    private:
        bool _activated;
    };

}