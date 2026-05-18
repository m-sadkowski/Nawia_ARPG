#pragma once

#include <Item.h>

namespace Nawia::Item {

    /**
     * @class Head
     * @brief Nakrycie glowy dodajace obrone.
     */
    class Head : public Item {
    public:
        Head(const int id, const std::string& name, const std::string& desc, const EquipmentSlot slot, 
            const std::shared_ptr<Texture2D>& icon, std::string model_path, const int defense)
            : Item(id, name, desc, slot, icon, model_path), _defense(defense) {
            _stats.defense = defense;
        }

        /** @brief Zwraca wartosc obrony. */
        [[nodiscard]] int getDefense() const { return _defense; }

        /** @brief Tworzy kopie nakrycia glowy z template'u. */
        [[nodiscard]] std::shared_ptr<Item> clone() const override {
            return std::make_shared<Head>(*this);
        }

    private:
        int _defense;
    };

} // namespace Nawia::Item
