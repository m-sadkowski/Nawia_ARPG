#include "Equipment.h"

namespace Nawia::Item {

    Equipment::Equipment(Core::ResourceManager& resource_manager) : _resource_manager(resource_manager) {
        _slots[EquipmentSlot::Head] = nullptr;
        _slots[EquipmentSlot::Neck] = nullptr;
        _slots[EquipmentSlot::Chest] = nullptr;
        _slots[EquipmentSlot::Legs] = nullptr;
        _slots[EquipmentSlot::Weapon] = nullptr;
        _slots[EquipmentSlot::OffHand] = nullptr;
        _slots[EquipmentSlot::Feet] = nullptr;
        _slots[EquipmentSlot::Ring] = nullptr;

		modelEmpty(EquipmentSlot::Feet);
		modelEmpty(EquipmentSlot::Legs);
		modelEmpty(EquipmentSlot::Chest);
    }

	Equipment::~Equipment() {
		_models.clear();
	}

    std::shared_ptr<Item> Equipment::equip(const std::shared_ptr<Item>& new_item) {
        if (!new_item) return nullptr;

        const EquipmentSlot target_slot = new_item->getSlot();
        if (target_slot == EquipmentSlot::None) return new_item;

		std::shared_ptr<Item> previous_item = unequip(target_slot);
        _slots[target_slot] = new_item;

		if (target_slot == EquipmentSlot::Weapon || target_slot == EquipmentSlot::OffHand)
			return previous_item;

		std::string model_path = new_item->getModelPath();
		if (!model_path.empty()) {
			Model* cloth = _resource_manager.getModel(model_path);
			if (cloth != nullptr) {
				_models[target_slot] = cloth;
			}
		}
		else if (hasBaseModel(target_slot)) {
			_models.erase(target_slot);
		}

        return previous_item;
    }

	bool Equipment::hasBaseModel(EquipmentSlot slot) const {
		return slot == EquipmentSlot::Feet
			|| slot == EquipmentSlot::Legs
			|| slot == EquipmentSlot::Chest;
	}

	void Equipment::modelEmpty(EquipmentSlot slot) {
		if (slot == EquipmentSlot::Feet) {
			Model* feet = _resource_manager.getModel("assets/models/player/player_feet.glb");
			if (feet != nullptr) {
				_models[EquipmentSlot::Feet] = feet;
			}
		}
		else if (slot == EquipmentSlot::Legs) {
			Model* legs = _resource_manager.getModel("assets/models/player/player_legs.glb");
			if (legs != nullptr) {
				_models[EquipmentSlot::Legs] = legs;
			}
		}
		else if (slot == EquipmentSlot::Chest) {
			Model* chest = _resource_manager.getModel("assets/models/player/player_body.glb");
			if (chest != nullptr) {
				_models[EquipmentSlot::Chest] = chest;
			}
		}
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
			if (hasBaseModel(slot))
				modelEmpty(slot);

            return item;
        }

        return nullptr;
    }

	void Equipment::updateAnimations(const ModelAnimation& current_anim, int frame) {
		for (auto& pair : _models) {
			UpdateModelAnimation(*(pair.second), current_anim, frame);
		}
	}

	void Equipment::draw(const Vector3 pos, const float owner_visual_rotation, const float owner_logical_rotation,
						 const float scale) {
		for (auto& pair : _models) {
			const bool body_slot = pair.first == EquipmentSlot::Head
				|| pair.first == EquipmentSlot::Chest
				|| pair.first == EquipmentSlot::Legs
				|| pair.first == EquipmentSlot::Feet;
			const float draw_rotation = body_slot ? owner_logical_rotation : owner_visual_rotation;
			DrawModelEx(*(pair.second), pos, {0, 1, 0}, draw_rotation, {scale, scale, scale}, WHITE);
		}
	}

} // namespace Nawia::Item
