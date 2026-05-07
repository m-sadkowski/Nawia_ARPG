#pragma once

#include <Item.h>

namespace Nawia::Item {

    /**
     * @class Weapon
     * @brief Przedmiot broni dodajacy obrazenia.
     */
    class Weapon : public Item {
    public:
        Weapon(const int id, const std::string& name, const std::string& desc, const EquipmentSlot slot, const std::shared_ptr<Texture2D>& icon, const int damage)
            : Item(id, name, desc, slot, icon), _damage(damage) {
            _stats.damage = damage;
        }

        /** @brief Zwraca bonus obrazen. */
        [[nodiscard]] int getDamage() const { return _damage; }

        /** @brief Tworzy kopie broni z template'u. */
        [[nodiscard]] std::shared_ptr<Item> clone() const override {
            return std::make_shared<Weapon>(*this);
        }

    private:
        int _damage;
    };

} // namespace Nawia::Item
