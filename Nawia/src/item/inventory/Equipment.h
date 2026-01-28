#pragma once
#include "Item.h"
#include <map>
#include <memory>

namespace Nawia::Item {

    class Equipment {
    public:
        Equipment();

        std::shared_ptr<Item> equip(const std::shared_ptr<Item>& new_item);
        std::shared_ptr<Item> unequip(EquipmentSlot slot);

        [[nodiscard]] std::shared_ptr<Item> getItemAt(EquipmentSlot slot) const;
        [[nodiscard]] const std::map<EquipmentSlot, std::shared_ptr<Item>>& getSlots() const { return _slots; }

    private:
        std::map<EquipmentSlot, std::shared_ptr<Item>> _slots;
    };
} // namespace Nawia::Item