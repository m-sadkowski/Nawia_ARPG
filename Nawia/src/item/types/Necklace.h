#pragma once

#include <Item.h>

namespace Nawia::Item {

    /**
     * @class Necklace
     * @brief Naszyjnik dodajacy inteligencje.
     */
    class Necklace : public Item {
    public:
        Necklace(const int id, const std::string& name, const std::string& desc, const EquipmentSlot slot, 
            const std::shared_ptr<Texture2D>& icon, const int intelligence)
            : Item(id, name, desc, slot, icon), _intelligence(intelligence) {}

        /** @brief Zwraca bonus inteligencji. */
        [[nodiscard]] int getIntelligence() const { return _intelligence; }

        /** @brief Tworzy kopie naszyjnika z template'u. */
        [[nodiscard]] std::shared_ptr<Item> clone() const override {
            return std::make_shared<Necklace>(*this);
        }

    private:
        int _intelligence;
    };

} // namespace Nawia::Item
