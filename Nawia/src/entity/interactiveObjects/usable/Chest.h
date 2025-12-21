#pragma once
#include "Usable.h"

namespace Nawia::Entity {

    class Chest : public Usable {
    public:
        Chest(float x, float y, const std::shared_ptr<SDL_Texture>& texture);

        void update(float delta_time) override;
        bool isInteractive() const override;
        void interaction() override;
    private:
        bool _isOpened;
    };

}