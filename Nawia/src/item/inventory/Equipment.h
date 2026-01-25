#pragma once
#include "Item.h"
#include <map>
#include <memory>

namespace Nawia::Item {

    class Equipment {
    public:
        Equipment();

        std::shared_ptr<Item> equip(std::shared_ptr<Item> newItem);
        std::shared_ptr<Item> unequip(EquipmentSlot slot);

        std::shared_ptr<Item> getItemAt(EquipmentSlot slot) const;


    private:
        std::map<EquipmentSlot, std::shared_ptr<Item>> _slots;
    };
}