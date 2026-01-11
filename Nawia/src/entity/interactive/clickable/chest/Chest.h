#pragma once
#include "InteractiveClickable.h"
#include "Entity.h"

namespace Nawia::Entity {

    class Chest : public InteractiveClickable {
    public:
        Chest(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture);

        // Implementujemy tylko to, co nas interesuje
        void onInteract(Entity& instigator) override;

        void update(float delta_time) override;
        void render(float offset_x, float offset_y) override;
        float getInteractionRange() override;
    private:
        bool _isOpen = false;
        // Mo¿esz tu dodaæ listê itemów: std::vector<Item> _loot;
    };

}