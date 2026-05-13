#pragma once

#include <Item.h>

namespace Nawia::Item {

    /**
     * @class Boots
     * @brief Buty dodajace obrone i predkosc ruchu.
     */
    class Boots : public Item {
    public:
        Boots(const int id, const std::string& name, const std::string& desc, const EquipmentSlot slot, const std::shared_ptr<Texture2D>& icon, std::string model_path,
            const int defense, const float movement_speed = 0.0f)
            : Item(id, name, desc, slot, icon, model_path), _defense(defense), _movement_speed(movement_speed) {
            _stats.defense = defense;
            _stats.movement_speed = movement_speed;
        }

        /** @brief Zwraca bonus obrony. */
        [[nodiscard]] int getDefense() const { return _defense; }

        /** @brief Zwraca bonus predkosci ruchu. */
        [[nodiscard]] float getMovementSpeed() const { return _movement_speed; }

        /** @brief Tworzy kopie butow z template'u. */
        [[nodiscard]] std::shared_ptr<Item> clone() const override {
            return std::make_shared<Boots>(*this);
        }

    private:
        int _defense;
        float _movement_speed;
    };

} // namespace Nawia::Item
