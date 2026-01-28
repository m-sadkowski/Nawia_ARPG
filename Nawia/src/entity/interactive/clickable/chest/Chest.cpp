#include "Chest.h"
#include <iostream>
#include <InteractiveClickable.h>

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

    void Chest::initializeInventory(Item::Loottable& loottable, Item::LOOTTABLE_TYPE loottable_type) {
        const auto& drops = loottable.getLootTable(loottable_type);

        for (const auto& entry : drops) {
            if (!entry._item) continue;

            float roll = static_cast<float>(GetRandomValue(0, 10000)) / 100.0f;

            if (roll <= entry._chance) {
                std::shared_ptr<Item::Item> uniqueItem = entry._item->clone();

                addItem(uniqueItem);
            }
        }
    }
    
    void Chest::onInteract(Entity& instigator) {
        if (_isOpen) {
            std::cout << "Skrzynia jest juz otwarta." << std::endl;
            return;
        }

        std::cout << instigator.getName() << " otwiera skrzynie " << _name << "!" << std::endl;

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
        return 2.f * 2.f;
    }

} // namespace Nawia::Entity
