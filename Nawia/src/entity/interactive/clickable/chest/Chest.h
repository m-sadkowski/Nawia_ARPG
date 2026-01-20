#pragma once
#include "InteractiveClickable.h"
#include "Entity.h"
#include "Backpack.h"

namespace Nawia::Entity {

    class Chest : public InteractiveClickable {
    public:
        Chest(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture);

        // Implementujemy tylko to, co nas interesuje
        void onInteract(Entity& instigator) override;

        void update(float delta_time) override;
        void render(float offset_x, float offset_y) override;
        float getInteractionRange() override;

        Item::Backpack& getInventory() { return *_inventory; }
        void addItem(std::shared_ptr<Item::Item> item) {
            _inventory->addItem(item);
        }
    private:
        bool _isOpen = false;
        std::unique_ptr<Item::Backpack> _inventory;
    };

}