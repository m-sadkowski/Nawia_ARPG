#pragma once
#include "Item.h"

namespace Nawia::Item {
    class Head : public Item {
    public:
        Head(int id, std::string name, std::string desc, EquipmentSlot slot, std::shared_ptr<Texture2D> icon, int armor)
            : Item(id, name, desc, slot, icon), _armor(armor) {
        }

        int getArmor() const { return _armor; }

        std::shared_ptr<Item> clone() const override {
            return std::make_shared<Head>(*this);
        }

    private:
        int _armor;
    };
}