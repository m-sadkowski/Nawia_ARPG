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
		Equipment(Core::ResourceManager& resource_manager);
		~Equipment();

        /**
         * @brief Zaklada przedmiot i zwraca poprzedni przedmiot ze slotu.
         */
		std::shared_ptr<Item> equip(const std::shared_ptr<Item>& new_item);

		void modelEmpty(EquipmentSlot slot);

        /**
         * @brief Zdejmuje przedmiot ze slotu i zwraca go do dalszej obslugi.
         */
        std::shared_ptr<Item> unequip(EquipmentSlot slot);

        /** @brief Zdejmuje wszystkie przedmioty bez zwracania ich do plecaka. */
        void clear();

        /** @brief Zwraca przedmiot w slocie albo nullptr. */
        [[nodiscard]] std::shared_ptr<Item> getItemAt(EquipmentSlot slot) const;

        /** @brief Zwraca mape slotow ekwipunku. */
        [[nodiscard]] const std::map<EquipmentSlot, std::shared_ptr<Item>>& getSlots() const { return _slots; }

		void updateAnimations(const ModelAnimation& current_anim, int frame);
		void draw(Vector3 pos, float owner_visual_rotation, float owner_logical_rotation, float scale);

    private:
		bool hasBaseModel(EquipmentSlot slot) const;

        std::map<EquipmentSlot, std::shared_ptr<Item>> _slots;

		Core::ResourceManager& _resource_manager;

		std::map<EquipmentSlot, Model*> _models;
    };

} // namespace Nawia::Item
