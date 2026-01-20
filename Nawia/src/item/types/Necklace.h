#pragma once
#include "Item.h"

namespace Nawia::Item {
    class Necklace : public Item {
    public:
        Necklace(int id, std::string name, std::string desc, EquipmentSlot slot, std::shared_ptr<Texture2D> icon, 
            int intelligence)
            : Item(id, name, desc, slot, icon), _intelligence(intelligence) {
        }

        int getIntelligence() const { return _intelligence; }

        std::shared_ptr<Item> clone() const override {
            return std::make_shared<Necklace>(*this);
        }

    private:
        int _intelligence;
    };
}