#pragma once

#include <Item.h>

namespace Nawia::Item {

    /**
     * @class Offhand
     * @brief Przedmiot do drugiej reki dodajacy obrazenia i obrone.
     */
    class Offhand : public Item {
    public:
        Offhand(const int id, const std::string& name, const std::string& desc, const EquipmentSlot slot,
            const std::shared_ptr<Texture2D>& icon, const int damage, const int defense)
            : Item(id, name, desc, slot, icon), _damage(damage), _defense(defense) {}

        /** @brief Zwraca bonus obrazen. */
        [[nodiscard]] int getDamage() const { return _damage; }

        /** @brief Zwraca bonus obrony. */
        [[nodiscard]] int getDefense() const { return _defense; }

        /** @brief Tworzy kopie przedmiotu z template'u. */
        [[nodiscard]] std::shared_ptr<Item> clone() const override {
            return std::make_shared<Offhand>(*this);
        }

    private:
        int _damage;
        int _defense;
    };

} // namespace Nawia::Item
