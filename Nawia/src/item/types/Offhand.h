#pragma once
#include "Item.h"

namespace Nawia::Item {

    class Offhand : public Item {
    public:
        Offhand(const int id, const std::string& name, const std::string& desc, const EquipmentSlot slot,
            const std::shared_ptr<Texture2D>& icon, const int damage, const int defense)
            : Item(id, name, desc, slot, icon), _damage(damage), _defense(defense) {}

        int getDamage() const { return _damage; }
        int getDefense() const { return _defense; }

        std::shared_ptr<Item> clone() const override {
            return std::make_shared<Offhand>(*this);
        }

    private:
        int _damage;
        int _defense;
    };

} // namespace Nawia::Item