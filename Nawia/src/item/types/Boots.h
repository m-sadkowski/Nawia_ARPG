#pragma once
#include "Item.h"

namespace Nawia::Item {
    class Boots : public Item {

    public:
        Boots(const int id, const std::string& name, const std::string& desc, const EquipmentSlot slot, const std::shared_ptr<Texture2D>& icon,
            const int defense, const float movement_speed = 0.0f)
            : Item(id, name, desc, slot, icon), _defense(defense), _movement_speed(movement_speed) 
    	{
            _stats.armor = defense;
            _stats.movement_speed = movement_speed;
        }

        int getDefense() const { return _defense; }
        float getMovementSpeed() const { return _movement_speed; }

        std::shared_ptr<Item> clone() const override {
            return std::make_shared<Boots>(*this);
        }

    private:
        int _defense;
        float _movement_speed;
    };

} // namespace Nawia::Item
