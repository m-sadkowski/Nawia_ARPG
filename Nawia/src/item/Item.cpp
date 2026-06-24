#include "Item.h"

#include <utility>

namespace Nawia::Item {

    Item::Item(const int id, std::string name, std::string description, const EquipmentSlot slot,
	           const std::shared_ptr<Texture2D>& icon, std::string model_path)
	    : _id(id), _name(std::move(name)), _description(std::move(description)), _slot(slot), _icon(icon),
	      _model_path(std::move(model_path)) {
	}

} // namespace Nawia::Item
