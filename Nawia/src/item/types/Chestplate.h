#pragma once
#include "Item.h"

namespace Nawia::Item {

    class Chestplate : public Item {

    public:
        Chestplate(const int id, const std::string& name, const std::string& desc, const EquipmentSlot slot, 
            const std::shared_ptr<Texture2D>& icon, const int defense)
            : Item(id, name, desc, slot, icon), _defense(defense) 
    	{
            _stats.tenacity = defense;
        }

        int getDefense() const { return _defense; }

        std::shared_ptr<Item> clone() const override {
            return std::make_shared<Chestplate>(*this);
        }

    private:
        int _defense;
    };

} // namespace Nawia::Item