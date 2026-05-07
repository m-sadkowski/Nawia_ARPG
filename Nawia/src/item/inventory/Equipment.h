#pragma once

#include <Item.h>
#include <ResourceManager.h>

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
		~Equipment();

        /**
         * @brief Zaklada przedmiot i zwraca poprzedni przedmiot ze slotu.
         */
		std::shared_ptr<Item> equip(const std::shared_ptr<Item>& new_item, Core::ResourceManager& resource_manager);

        /**
         * @brief Zdejmuje przedmiot ze slotu i zwraca go do dalszej obslugi.
         */
        std::shared_ptr<Item> unequip(EquipmentSlot slot);

        /** @brief Zwraca przedmiot w slocie albo nullptr. */
        [[nodiscard]] std::shared_ptr<Item> getItemAt(EquipmentSlot slot) const;

        /** @brief Zwraca mape slotow ekwipunku. */
        [[nodiscard]] const std::map<EquipmentSlot, std::shared_ptr<Item>>& getSlots() const { return _slots; }

		void updateAnimations(const ModelAnimation& current_anim, int frame);
		void draw(Vector3 pos, float rotation_angle, float scale);

    private:
        std::map<EquipmentSlot, std::shared_ptr<Item>> _slots;

		std::map<EquipmentSlot, Model*> _models;
    };

} // namespace Nawia::Item
