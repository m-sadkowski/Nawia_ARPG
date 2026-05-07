#include "Equipment.h"

#include "Logger.h"

namespace Nawia::Item {

    Equipment::Equipment() {
        _slots[EquipmentSlot::Head] = nullptr;
        _slots[EquipmentSlot::Neck] = nullptr;
        _slots[EquipmentSlot::Chest] = nullptr;
        _slots[EquipmentSlot::Legs] = nullptr;
        _slots[EquipmentSlot::Weapon] = nullptr;
        _slots[EquipmentSlot::OffHand] = nullptr;
        _slots[EquipmentSlot::Feet] = nullptr;
        _slots[EquipmentSlot::Ring] = nullptr;
    }

	Equipment::~Equipment() {
		_models.clear();
	}

    std::shared_ptr<Item> Equipment::equip(const std::shared_ptr<Item>& new_item,
	                                       Core::ResourceManager& resource_manager) {
        if (!new_item) return nullptr;

        const EquipmentSlot target_slot = new_item->getSlot();
        if (target_slot == EquipmentSlot::None) return new_item;

        std::shared_ptr<Item> previous_item = unequip(target_slot);
        _slots[target_slot] = new_item;

		std::string model_path = new_item->getModelPath();
		if (!model_path.empty()) {
			Model* cloth = resource_manager.getModel(model_path);
			if (cloth != nullptr) {
				_models[target_slot] = cloth;
			}
		}

        return previous_item;
    }

    std::shared_ptr<Item> Equipment::getItemAt(const EquipmentSlot slot) const {
        if (_slots.count(slot)) return _slots.at(slot);
        return nullptr;
    }

    std::shared_ptr<Item> Equipment::unequip(const EquipmentSlot slot) {
		if (_slots.find(slot) != _slots.end() && _slots[slot] != nullptr) {
            auto item = _slots[slot];
            _slots[slot] = nullptr;

			auto model_it = _models.find(slot);
			if (model_it != _models.end()) {
				_models.erase(model_it);
			}

            return item;
        }

        return nullptr;
    }

	void Equipment::updateAnimations(const ModelAnimation& current_anim, int frame) {
		for (auto& pair : _models) {
			if (pair.first == EquipmentSlot::Weapon || pair.first == EquipmentSlot::OffHand) {
				continue;
			}
			UpdateModelAnimation(*(pair.second), current_anim, frame);
		}
	}

	void Equipment::draw(Vector3 pos, float rotation_angle, float scale) {
		for (auto& pair : _models) {
			DrawModelEx(*(pair.second), pos, {0, 1, 0}, rotation_angle, {scale, scale, scale}, WHITE);
		}
	}

} // namespace Nawia::Item
