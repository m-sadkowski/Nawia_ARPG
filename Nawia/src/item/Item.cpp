#include "Item.h"

namespace Nawia::Item {

    Item::Item(int id, std::string name, std::string description, EquipmentSlot slot, std::shared_ptr<Texture2D> icon)
        : _id(id),
        _name(std::move(name)),
        _description(std::move(description)),
        _slot(slot),
        _icon(icon)
    {}

}