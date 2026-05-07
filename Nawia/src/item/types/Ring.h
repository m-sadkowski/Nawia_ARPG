#pragma once

#include <Item.h>

namespace Nawia::Item {

    /**
     * @class Ring
     * @brief Pierscien dodajacy inteligencje.
     */
    class Ring : public Item {
    public:
        Ring(const int id, const std::string& name, const std::string& desc, const EquipmentSlot slot, 
            const std::shared_ptr<Texture2D>& icon, std::string model_path, const int intelligence)
            : Item(id, name, desc, slot, icon, model_path), _intelligence(intelligence) {}

        /** @brief Zwraca bonus inteligencji. */
        [[nodiscard]] int getIntelligence() const { return _intelligence; }

        /** @brief Tworzy kopie pierscienia z template'u. */
        [[nodiscard]] std::shared_ptr<Item> clone() const override {
            return std::make_shared<Ring>(*this);
        }

    private:
        int _intelligence;
    };

} // namespace Nawia::Item
