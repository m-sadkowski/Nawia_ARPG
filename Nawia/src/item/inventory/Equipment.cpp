#include "Equipment.h"

namespace Nawia::Item {

    Equipment::Equipment() {
        _slots[EquipmentSlot::Head] = nullptr;
        _slots[EquipmentSlot::Chest] = nullptr;
        _slots[EquipmentSlot::Weapon] = nullptr;
        _slots[EquipmentSlot::OffHand] = nullptr;
        _slots[EquipmentSlot::Feet] = nullptr;
    }

    std::shared_ptr<Item> Equipment::equip(std::shared_ptr<Item> newItem) {
        if (!newItem) return nullptr;

        EquipmentSlot targetSlot = newItem->getSlot();
        if (targetSlot == EquipmentSlot::None) return newItem; // cant equip

        // check what is equiped
        std::shared_ptr<Item> oldItem = _slots[targetSlot];

        // change old -> new
        _slots[targetSlot] = newItem;

        // return old so it goes back to backpack
        // or null if nothing was there
        return oldItem;
    }

    std::shared_ptr<Item> Equipment::getItemAt(EquipmentSlot slot) const {
        if (_slots.count(slot)) return _slots.at(slot);
        return nullptr;
    }
}