#include "Equipment.h"

namespace Nawia::Item {

	namespace {
		constexpr EquipmentSlot EQUIPMENT_SLOTS[] = {
			EquipmentSlot::Head,
			EquipmentSlot::Neck,
			EquipmentSlot::Chest,
			EquipmentSlot::Legs,
			EquipmentSlot::Weapon,
			EquipmentSlot::OffHand,
			EquipmentSlot::Feet,
			EquipmentSlot::Ring
		};

		const char* baseModelPath(const EquipmentSlot slot) {
			switch (slot) {
				case EquipmentSlot::Feet: return "assets/models/actors/player/parts/player_feet.glb";
				case EquipmentSlot::Legs: return "assets/models/actors/player/parts/player_legs.glb";
				case EquipmentSlot::Chest: return "assets/models/actors/player/parts/player_body.glb";
				default: return nullptr;
			}
		}
	}

    Equipment::Equipment(Core::ResourceManager& resource_manager) : _resource_manager(resource_manager) {
		for (const EquipmentSlot slot : EQUIPMENT_SLOTS)
			_slots[slot] = nullptr;

		for (const EquipmentSlot slot : {EquipmentSlot::Feet, EquipmentSlot::Legs, EquipmentSlot::Chest})
			modelEmpty(slot);
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
		return baseModelPath(slot) != nullptr;
	}

	void Equipment::modelEmpty(EquipmentSlot slot) {
		const char* path = baseModelPath(slot);
		if (!path)
			return;

		if (Model* model = _resource_manager.getModel(path))
			_models[slot] = model;
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

	void Equipment::clear() {
		for (auto& [slot, item] : _slots)
			item = nullptr;

		_models.clear();
		for (const EquipmentSlot slot : {EquipmentSlot::Feet, EquipmentSlot::Legs, EquipmentSlot::Chest})
			modelEmpty(slot);
	}

	void Equipment::updateAnimations(const ModelAnimation& current_anim, int frame) {
		for (auto& pair : _models) {
			UpdateModelAnimation(*(pair.second), current_anim, frame);
		}
	}

	void Equipment::draw(const Vector3 pos, const float owner_visual_rotation, const float /*owner_logical_rotation*/,
						 const float scale) {
		// Wszystkie elementy ekwipunku rysowane sa z ta sama rotacja co model wlasciciela.
		// Entity::render uzywa visual_rotation (_rotation + _model_facing_offset), wiec
		// ubrania musza uzywac dokladnie tej samej wartosci, aby nie rozjezdzaly sie
		// z modelem postaci.
		for (auto& pair : _models) {
			DrawModelEx(*(pair.second), pos, {0, 1, 0}, owner_visual_rotation, {scale, scale, scale}, WHITE);
		}
	}

	nlohmann::json Equipment::serialize() const {
		nlohmann::json result = nlohmann::json::array();

		for (const auto& [slot, item] : _slots) {
			if (!item)
				continue;

			result.push_back({
				{"slot", slotToString(slot)},
				{"item_id", item->getId()}
			});
		}

		return result;
	}

	std::string Equipment::slotToString(const EquipmentSlot slot) {
		switch (slot) {
			case EquipmentSlot::Head: return "Head";
			case EquipmentSlot::Neck: return "Neck";
			case EquipmentSlot::Chest: return "Chest";
			case EquipmentSlot::Legs: return "Legs";
			case EquipmentSlot::Feet: return "Feet";
			case EquipmentSlot::Weapon: return "Weapon";
			case EquipmentSlot::OffHand: return "OffHand";
			case EquipmentSlot::Ring: return "Ring";
			case EquipmentSlot::None: return "None";
		}

		return "None";
	}

	EquipmentSlot Equipment::slotFromString(const std::string& slot_name) {
		if (slot_name == "Head") return EquipmentSlot::Head;
		if (slot_name == "Neck") return EquipmentSlot::Neck;
		if (slot_name == "Chest") return EquipmentSlot::Chest;
		if (slot_name == "Legs") return EquipmentSlot::Legs;
		if (slot_name == "Feet") return EquipmentSlot::Feet;
		if (slot_name == "Weapon") return EquipmentSlot::Weapon;
		if (slot_name == "OffHand") return EquipmentSlot::OffHand;
		if (slot_name == "Ring") return EquipmentSlot::Ring;
		return EquipmentSlot::None;
	}

} // namespace Nawia::Item
