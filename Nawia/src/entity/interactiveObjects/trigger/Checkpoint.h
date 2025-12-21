#pragma once
#include "InteractiveObject.h"

namespace Nawia::Entity {

    class Checkpoint : public InteractiveObject {
    public:
        Checkpoint(float x, float y, const std::shared_ptr<SDL_Texture>& texture);

        void update(float delta_time) override;


        bool isInteractive() const override;
        void interaction() override;


        


        float getInteractionRange() const override { return 0.5f; }

    private:
        bool _activated;
    };

}