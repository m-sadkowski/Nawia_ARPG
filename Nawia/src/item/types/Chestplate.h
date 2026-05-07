#pragma once

#include <Item.h>

namespace Nawia::Item {

    /**
     * @class Chestplate
     * @brief Pancerz na klatke piersiowa dodajacy wytrzymalosc.
     */
    class Chestplate : public Item {

    public:
        Chestplate(const int id, const std::string& name, const std::string& desc, const EquipmentSlot slot, 
            const std::shared_ptr<Texture2D>& icon, std::string model_path, const int defense)
            : Item(id, name, desc, slot, icon, model_path), _defense(defense) 
    	{
            _stats.tenacity = defense;
        }

        /** @brief Zwraca wartosc obrony. */
        [[nodiscard]] int getDefense() const { return _defense; }

        /** @brief Tworzy kopie pancerza z template'u. */
        [[nodiscard]] std::shared_ptr<Item> clone() const override {
            return std::make_shared<Chestplate>(*this);
        }

    private:
        int _defense;
    };

} // namespace Nawia::Item
