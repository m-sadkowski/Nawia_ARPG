#include "Equipment.h"

#include "Logger.h"

#include <raymath.h>

#include <cstring>

namespace {
	constexpr const char* WEAPON_HAND_BONE = "hand_r";
	constexpr Vector3 WEAPON_GRIP_ROTATION_DEG = {0.0f, 0.0f, 90.0f};
	constexpr Vector3 WEAPON_GRIP_OFFSET = {-0.06f, 0.22f, 0.34f};
	constexpr float WEAPON_GRIP_SCALE = 0.25f;

	Matrix transformToMatrix(const Transform& transform) {
		const Matrix mat_scale = MatrixScale(transform.scale.x, transform.scale.y, transform.scale.z);
		const Matrix mat_rotate = QuaternionToMatrix(transform.rotation);
		const Matrix mat_translate = MatrixTranslate(transform.translation.x, transform.translation.y, transform.translation.z);
		return MatrixMultiply(MatrixMultiply(mat_scale, mat_rotate), mat_translate);
	}
}

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
			if (pair.first == EquipmentSlot::Weapon || pair.first == EquipmentSlot::OffHand) {
				continue;
			}
			UpdateModelAnimation(*(pair.second), current_anim, frame);
		}
	}

	Matrix Equipment::getOwnerWorldTransform(const Vector3 pos, const float rotation_angle, const float scale,
											 const Model& owner_model) const {
		const Matrix mat_translate = MatrixTranslate(pos.x, pos.y, pos.z);
		const Matrix mat_rotate = MatrixRotate({0.0f, 1.0f, 0.0f}, rotation_angle * DEG2RAD);
		const Matrix mat_scale = MatrixScale(scale, scale, scale);
		return MatrixMultiply(
			MatrixMultiply(MatrixMultiply(mat_scale, owner_model.transform), mat_rotate),
			mat_translate);
	}

	int Equipment::findBoneIndex(const ModelAnimation& animation, const char* bone_name) const {
		if (animation.bones == nullptr || bone_name == nullptr)
			return -1;

		for (int i = 0; i < animation.boneCount; ++i) {
			if (std::strcmp(animation.bones[i].name, bone_name) == 0)
				return i;
		}

		return -1;
	}

	Matrix Equipment::getBoneWorldTransform(const ModelAnimation& animation, int frame, const int bone_index,
											const Model& owner_model) const {
		if (animation.frameCount > 0)
			frame %= animation.frameCount;

		Transform pose = animation.framePoses[frame][bone_index];
		if (owner_model.bindPose != nullptr && owner_model.meshCount > 0 &&
			owner_model.meshes[0].boneMatrices != nullptr && bone_index < owner_model.meshes[0].boneCount) {
			pose.translation = Vector3Transform(owner_model.bindPose[bone_index].translation,
												owner_model.meshes[0].boneMatrices[bone_index]);
		}

		return transformToMatrix(pose);
	}

	bool Equipment::tryDrawAttachedWeapon(Model& model, const ModelAnimation& current_anim, const int frame,
										  const Model& owner_model, const Matrix& owner_world_transform) const {
		if (current_anim.frameCount <= 0 || current_anim.bones == nullptr || current_anim.framePoses == nullptr)
			return false;

		const int hand_bone_index = findBoneIndex(current_anim, WEAPON_HAND_BONE);
		if (hand_bone_index < 0) {
			Core::Logger::debugLog(std::string("Equipment: nie znaleziono kosci broni: ") + WEAPON_HAND_BONE);
			return false;
		}

		const Matrix hand_world_transform = MatrixMultiply(
			getBoneWorldTransform(current_anim, frame, hand_bone_index, owner_model),
			owner_world_transform);

		// Lokalna korekta chwytu miecza. Te wartosci sa do strojenia pod konkretny sword.glb.
		const Matrix grip_offset = MatrixMultiply(
			MatrixMultiply(
				MatrixRotateXYZ({
					WEAPON_GRIP_ROTATION_DEG.x * DEG2RAD,
					WEAPON_GRIP_ROTATION_DEG.y * DEG2RAD,
					WEAPON_GRIP_ROTATION_DEG.z * DEG2RAD}),
				MatrixScale(WEAPON_GRIP_SCALE, WEAPON_GRIP_SCALE, WEAPON_GRIP_SCALE)),
			MatrixTranslate(WEAPON_GRIP_OFFSET.x, WEAPON_GRIP_OFFSET.y, WEAPON_GRIP_OFFSET.z));

		const Matrix original_transform = model.transform;
		model.transform = MatrixMultiply(MatrixMultiply(original_transform, grip_offset), hand_world_transform);
		DrawModel(model, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
		model.transform = original_transform;
		return true;
	}

	void Equipment::draw(const Vector3 pos, const float rotation_angle, const float scale, const Model& owner_model,
						 const ModelAnimation* current_anim, const int frame) {
		const Matrix owner_world_transform = getOwnerWorldTransform(pos, rotation_angle, scale, owner_model);
		for (auto& pair : _models) {
			if (pair.first == EquipmentSlot::Weapon || pair.first == EquipmentSlot::OffHand) {
				if (current_anim != nullptr && tryDrawAttachedWeapon(*(pair.second), *current_anim, frame, owner_model,
																	 owner_world_transform))
					continue;

				continue;
			}

			DrawModelEx(*(pair.second), pos, {0, 1, 0}, rotation_angle, {scale, scale, scale}, WHITE);
		}
	}

} // namespace Nawia::Item
