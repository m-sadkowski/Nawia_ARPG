#include "Item.h"

namespace Nawia::Item {

    Item::Item(const int id, std::string name, std::string description, const EquipmentSlot slot, const std::shared_ptr<Texture2D>& icon)
        : _id(id), _name(std::move(name)), _description(std::move(description)), _slot(slot), _icon(icon) {}

}