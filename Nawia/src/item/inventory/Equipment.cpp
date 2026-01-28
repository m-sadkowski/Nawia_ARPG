#include "Equipment.h"

namespace Nawia::Item {

    Equipment::Equipment()
	{
        _slots[EquipmentSlot::Head] = nullptr;
        _slots[EquipmentSlot::Chest] = nullptr;
        _slots[EquipmentSlot::Weapon] = nullptr;
        _slots[EquipmentSlot::OffHand] = nullptr;
        _slots[EquipmentSlot::Feet] = nullptr;
    }

    std::shared_ptr<Item> Equipment::equip(const std::shared_ptr<Item>& new_item)
	{
        if (!new_item) return nullptr;

        const EquipmentSlot target_slot = new_item->getSlot();
        if (target_slot == EquipmentSlot::None) return new_item; // cant equip

        // check what is equiped
        std::shared_ptr<Item> oldItem = _slots[target_slot];

        // change old -> new
        _slots[target_slot] = new_item;

        // return old so it goes back to backpack
        // or null if nothing was there
        return oldItem;
    }

    std::shared_ptr<Item> Equipment::getItemAt(const EquipmentSlot slot) const 
	{
        if (_slots.count(slot)) return _slots.at(slot);
			return nullptr;
    }

    std::shared_ptr<Item> Equipment::unequip(const EquipmentSlot slot) 
	{
        if (_slots.find(slot) != _slots.end()) 
        {
            auto item = _slots[slot];
            _slots[slot] = nullptr;
            return item;
        }

        return nullptr;
    }
} // namespace Nawia::Item