#pragma once
#include "InteractiveClickable.h"
#include "Entity.h"
#include "Backpack.h"
#include "Loottable.h"

namespace Nawia::Entity {

    class Chest : public InteractiveClickable {
    public:
        Chest(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture);

        void initializeInventory(Item::Loottable& loot_table, Item::LOOTTABLE_TYPE loot_table_type);

        void onInteract(Entity& instigator) override;

        void update(float delta_time) override;
        void render(float offset_x, float offset_y) override;
        float getInteractionRange() override;

        Item::Backpack* getInventory() override { 
            if (_locked) return nullptr;
            return _inventory.get(); 
        }

        void addItem(const std::shared_ptr<Item::Item>& item) {
            _inventory->addItem(item);
        }

        void setLocked(const bool locked, const int key_id) {
            _locked = locked;
            _key_id = key_id;
        }

    private:
        bool _isOpen = false;
        bool _locked = false;
        int _key_id = -1;

        std::unique_ptr<Item::Backpack> _inventory;
        static constexpr int CHEST_INV_SIZE = 12;
    public:
        
    };

}