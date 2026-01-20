#pragma once
#include "Item.h"

namespace Nawia::Item {
    class Legs : public Item {
    public:
        Legs(int id, std::string name, std::string desc, EquipmentSlot slot, std::shared_ptr<Texture2D> icon, 
            int defense)
            : Item(id, name, desc, slot, icon), _defense(defense) {
        }

        int getDefense() const { return _defense; }

        std::shared_ptr<Item> clone() const override {
            return std::make_shared<Legs>(*this);
        }

    private:
        int _defense;
    };
}