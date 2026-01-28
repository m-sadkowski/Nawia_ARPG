#include "Cat.h"
#include <iostream>
#include <InteractiveClickable.h>
#include "Player.h"
#include "Engine.h"

#include "Collider.h"
#include "Logger.h"

namespace Nawia::Entity {

    Cat::Cat(const std::string& name, const float x, const float y, const std::shared_ptr<Texture2D>& texture)
        : InteractiveClickable(name, x, y, texture, 1) // 1 HP
    {
        _type = EntityType::NPCStatic;
        setFaction(Faction::None);
        this->setScale(0.03f);
        loadModel("../assets/models/cat_bounce.glb", false);
        setCollider(std::make_unique<RectangleCollider>(this, 0.8f, 0.4f, -1.6f, -0.8f));
        playAnimation("default");

        _inventory = std::make_unique<Item::Backpack>(_inv_size);
    }

    void Cat::initializeInventory(Item::Loottable& lootable, const Item::LOOTTABLE_TYPE lootable_type) const 
	{
        const auto& drops = lootable.getLootTable(lootable_type);

        for (const auto& entry : drops) 
        {
            if (!entry._item) continue;

            const float roll = static_cast<float>(GetRandomValue(0, 10000)) / 100.0f;

            if (roll <= entry._chance) {
                const std::shared_ptr<Item::Item> unique_item = entry._item->clone();

                addItem(unique_item);
            }
        }
    } 

    void Cat::onInteract(Entity& instigator) 
	{
        if (_quest_completed) return;

        if (auto* player = dynamic_cast<Player*>(&instigator))
        {
             // Check for Fish (ID 6)
             int fish_index = -1;
             auto& backpack = player->getBackpack();
             const auto& items = backpack.getItems();
             for(int i = 0; i < items.size(); ++i) 
             {
                 if (items[i] && items[i]->getId() == 6) 
                 {
                     fish_index = i;
                     break;
                 }
             }

             if (fish_index != -1)
             {
                 // Remove fish
                 backpack.removeItem(fish_index);
                 
                 // Give Cat Sword (ID 7)
                 if (const auto sword = player->getEngine()->getItemDatabase().createItem(7))
                    this->addItem(sword);

                 _quest_completed = true;
                 
                  player->getEngine()->getDialogueManager().createCatQuestCompletedDialogue(player->getEngine(), this);
                 
                 if (player) 
                 {
                    player->getEngine()->getUIHandler().showNotification("Zadanie ukonczone! Otrzymano Miecz Kota.", 4.0f);
                 }

                 return;
             }
        }

        if (_isOpen)
            return;

        _isOpen = true;
    }

    void Cat::update(const float delta_time) 
	{
        Entity::update(delta_time);
    }

    void Cat::render(const float offset_x, const float offset_y)
	{
        Entity::render(offset_x, offset_y);
    }

    float Cat::getInteractionRange()
    {
        return 2.5f * 2.5f;
    }

} // namespace Nawia::Entity