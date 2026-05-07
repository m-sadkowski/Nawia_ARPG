#pragma once

#include <Item.h>

#include <map>
#include <memory>

namespace Nawia::Item {

    /**
     * @class Equipment
     * @brief Przechowuje zalozone przedmioty wedlug slotow ekwipunku.
     */
    class Equipment {
    public:
        Equipment();

        /**
         * @brief Zaklada przedmiot i zwraca poprzedni przedmiot ze slotu.
         */
        std::shared_ptr<Item> equip(const std::shared_ptr<Item>& new_item);

        /**
         * @brief Zdejmuje przedmiot ze slotu i zwraca go do dalszej obslugi.
         */
        std::shared_ptr<Item> unequip(EquipmentSlot slot);

        /** @brief Zwraca przedmiot w slocie albo nullptr. */
        [[nodiscard]] std::shared_ptr<Item> getItemAt(EquipmentSlot slot) const;

        /** @brief Zwraca mape slotow ekwipunku. */
        [[nodiscard]] const std::map<EquipmentSlot, std::shared_ptr<Item>>& getSlots() const { return _slots; }

    private:
        std::map<EquipmentSlot, std::shared_ptr<Item>> _slots;
    };

} // namespace Nawia::Item
