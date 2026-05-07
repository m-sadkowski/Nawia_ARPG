#pragma once

#include <Item.h>

namespace Nawia::Item {

    /**
     * @class Legs
     * @brief Spodnie albo nogawice dodajace obrone.
     */
    class Legs : public Item {
    public:
        Legs(const int id, const std::string& name, const std::string& desc, const EquipmentSlot slot, 
            const std::shared_ptr<Texture2D>& icon, std::string model_path, const int defense)
            : Item(id, name, desc, slot, icon, model_path), _defense(defense) {}

        /** @brief Zwraca wartosc obrony. */
        [[nodiscard]] int getDefense() const { return _defense; }

        /** @brief Tworzy kopie nogawic z template'u. */
        [[nodiscard]] std::shared_ptr<Item> clone() const override {
            return std::make_shared<Legs>(*this);
        }

    private:
        int _defense;
    };
     
} // namespace Nawia::Item
