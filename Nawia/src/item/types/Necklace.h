#pragma once
#include "Item.h"

namespace Nawia::Item {

    class Necklace : public Item {
    public:
        Necklace(const int id, const std::string& name, const std::string& desc, const EquipmentSlot slot, 
            const std::shared_ptr<Texture2D>& icon, const int intelligence)
            : Item(id, name, desc, slot, icon), _intelligence(intelligence) {}

        int getIntelligence() const { return _intelligence; }

        std::shared_ptr<Item> clone() const override {
            return std::make_shared<Necklace>(*this);
        }

    private:
        int _intelligence;
    };

} // namespace Nawia::Item