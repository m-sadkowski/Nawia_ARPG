#include "Cat.h"
#include <iostream>
#include <InteractiveClickable.h>

#include "Collider.h"
#include "Logger.h"

namespace Nawia::Entity {

    Cat::Cat(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture)
        : InteractiveClickable(name, x, y, texture, 1) // 1 HP
    {
        _type = EntityType::NPCStatic;
        setFaction(Faction::None);
        this->setScale(0.08f);
        loadModel("../assets/models/cat_bounce.glb", false);
        setCollider(std::make_unique<RectangleCollider>(this, 0.8f, 0.4f, -1.3f, -0.5f));
        playAnimation("default");

        _inventory = std::make_unique<Item::Backpack>(_inv_size);
    }

    void Cat::initializeInventory(Item::Loottable& loottable, Item::LOOTTABLE_TYPE loottable_type) {
        const auto& drops = loottable.getLoottable(loottable_type);

        Core::Logger::debugLog("XXXXXXXXXXXXXX");
        Core::Logger::debugLog("XXXXXXXXXXXXXX");
        Core::Logger::debugLog("XXXXXXXXXXXXXX");

        for (const auto& entry : drops) {
            if (!entry._item) continue;

            Core::Logger::debugLog("XXXXXXXXXXXXXX");
            Core::Logger::debugLog("XXXXXXXXXXXXXX");
            Core::Logger::debugLog("XXXXXXXXXXXXXX");

            float roll = static_cast<float>(GetRandomValue(0, 10000)) / 100.0f;

            if (roll <= entry._chance) {
                std::shared_ptr<Item::Item> uniqueItem = entry._item->clone();

                addItem(uniqueItem);
            }
        }
    }

    void Cat::onInteract(Entity& instigator) {
        if (_isOpen) {
            return;
        }
        _isOpen = true;
    }

    void Cat::update(float delta_time) {
        Entity::update(delta_time);
    }

    void Cat::render(float offset_x, float offset_y) {
        Entity::render(offset_x, offset_y);

        if (!_isOpen) {
            // Można dodać ikonkę wykrzyknika nad skrzynią
        }
    }

    float Cat::getInteractionRange()
    {
        return 2.5f * 2.5f;
    }

} // namespace Nawia::Entity