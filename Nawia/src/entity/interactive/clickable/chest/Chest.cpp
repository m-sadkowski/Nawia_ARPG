#include "Chest.h"
#include <iostream>
#include <InteractiveClickable.h>
#include "Player.h"
#include "Engine.h"

#include "Collider.h"

namespace Nawia::Entity {

    Chest::Chest(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture)
        : InteractiveClickable(name, x, y, texture, 1) // 1 HP
    {
        _type = EntityType::Chest;
        setFaction(Faction::None);
        _use_3d_rendering = false;
        setCollider(std::make_unique<RectangleCollider>(this, 0.9f, 0.4f, -0.5f, -0.5f));

        _inventory = std::make_unique<Item::Backpack>(CHEST_INV_SIZE);
    }

    void Chest::initializeInventory(Item::Loottable& loot_table, const Item::LOOTTABLE_TYPE loot_table_type) {
        const auto& drops = loot_table.getLootTable(loot_table_type);

        for (const auto& entry : drops) {
            if (!entry._item) continue;

            float roll = static_cast<float>(GetRandomValue(0, 10000)) / 100.0f;

            if (roll <= entry._chance) {
                std::shared_ptr<Item::Item> unique_item = entry._item->clone();

                addItem(unique_item);
            }
        }
    }
    
    void Chest::onInteract(Entity& instigator) 
	{
        if (_isOpen) 
        {
            if (const auto* player = dynamic_cast<Player*>(&instigator))
                player->getEngine()->getUIHandler().showNotification("Skrzynia jest juz otwarta.");
            
            return;
        }

        if (_locked) 
        {
             if (auto* player = dynamic_cast<Player*>(&instigator)) 
             {
                 bool has_key = false;
                 int key_index = -1;
                 auto& backpack = player->getBackpack();
                 const auto& items = backpack.getItems();
                 for(int i = 0; i < items.size(); ++i) {
                     if (items[i] && items[i]->getId() == 5) {
                         key_index = i;
                         has_key = true;
                         break;
                     }
                 }

                 if (has_key) 
                 {
                     backpack.removeItem(key_index);
                     auto& ui = player->getEngine()->getUIHandler();
                     ui.showNotification("Skrzynia otwarta! Zuzyto Klucz Kota.", 2.5f);
                     _locked = false;
                 } 
             	 else {
                     auto& ui = player->getEngine()->getUIHandler();
                     ui.showNotification("Skrzynia zamknieta! Potrzebny Klucz Kota.", 3.0f);
                     return;
                 }
             } 
        	 else 
        	 {
        		 return;
        	 }
        }

        // playAnimation("open_chest", false);

        _isOpen = true;

        // instigator.addItem(this->getLoot());
    }

    void Chest::update(const float delta_time) 
	{
        Entity::update(delta_time);
    }

    void Chest::render(const float offset_x, const float offset_y) 
	{
        Entity::render(offset_x, offset_y);

    }

    float Chest::getInteractionRange()
    {
        return 2.5f * 2.5f;
    }

} // namespace Nawia::Entity
