#pragma once
#include "Item.h"

namespace Nawia::Item {

    class Legs : public Item {
    public:
        Legs(const int id, const std::string& name, const std::string& desc, const EquipmentSlot slot, 
            const std::shared_ptr<Texture2D>& icon, const int defense)
            : Item(id, name, desc, slot, icon), _defense(defense) {}

        int getDefense() const { return _defense; }

        std::shared_ptr<Item> clone() const override {
            return std::make_shared<Legs>(*this);
        }

    private:
        int _defense;
    };
     
} // namespace Nawia::Item