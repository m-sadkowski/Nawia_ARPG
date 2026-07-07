#include "Entity.h"

#include <EntityAnimationSupport.h>
#include <EntityModelState.h>
#include <Logger.h>
#include <ResourceManager.h>

#include <raymath.h>

namespace Nawia::Entity {

	void Entity::unloadModelData()
	{
		_model_state->animations.clear();
		_model_state->animation_map.clear();
		_model_state->animation_path_map.clear();

		if (_model_state->model_loaded && _model_state->owns_model) {
			if (_model_state->cloned_model)
				Core::ResourceManager::unloadClonedModel(_model_state->model);
			else
				UnloadModel(_model_state->model);
		}

		_model_state->model = {};
		_model_state->model_loaded = false;
		_model_state->owns_model = false;
		_model_state->cloned_model = false;
		_model_state->local_model_bounding_box = {};
		_model_state->local_model_bounding_box_valid = false;
		_model_state->current_anim_index = 0;
		_model_state->anim_frame_counter = 0.0f;
		_model_state->last_applied_anim_index = -1;
		_model_state->last_applied_anim_frame = -1;
		_model_state->anim_looping = true;
		_model_state->anim_locked = false;
		_model_state->anim_ping_pong = false;
		_model_state->anim_reverse_phase = false;
		_model_state->anim_direction = 1.0f;
	}

	void Entity::loadModel(const std::string& path, const bool rotate_model)
	{
		unloadModelData();

		if (Core::ResourceManager* resource_manager = getSharedResourceManager()) {
			Model cloned = resource_manager->cloneModel(path);
			if (cloned.meshCount > 0) {
				if (rotate_model)
					cloned.transform = MatrixRotateX(-PI / 2.0f);

				_model_state->model = cloned;
				ensureMeshAnimationBuffers(_model_state->model);
				_model_state->model_loaded = true;
				_model_state->owns_model = true;
				_model_state->cloned_model = true;
				_model_state->local_model_bounding_box = GetModelBoundingBox(_model_state->model);
				_model_state->local_model_bounding_box_valid = true;
				_model_state->last_applied_anim_index = -1;
				_model_state->last_applied_anim_frame = -1;
				addAnimation("default", path);
				return;
			}
		}

		replaceModel(path, rotate_model);
		if (_model_state->model_loaded)
			addAnimation("default", path);
	}

	void Entity::replaceModel(const std::string& path, const bool rotate_model)
	{
		if (_model_state->model_loaded && _model_state->owns_model) {
			if (_model_state->cloned_model)
				Core::ResourceManager::unloadClonedModel(_model_state->model);
			else
				UnloadModel(_model_state->model);
		}
		_model_state->cloned_model = false;

		_model_state->model = LoadModel(path.c_str());
		if (_model_state->model.meshCount == 0)
		{
			_model_state->model = {};
			_model_state->model_loaded = false;
			_model_state->owns_model = false;
			_model_state->local_model_bounding_box = {};
			_model_state->local_model_bounding_box_valid = false;
			Core::Logger::errorLog("Nie udalo sie zaladowac modelu: " + path);
			return;
		}

		if (rotate_model)
			_model_state->model.transform = MatrixRotateX(-PI / 2.0f);

		ensureMeshAnimationBuffers(_model_state->model);

		_model_state->model_loaded = true;
		_model_state->owns_model = true;
		_model_state->local_model_bounding_box = GetModelBoundingBox(_model_state->model);
		_model_state->local_model_bounding_box_valid = true;
		_model_state->last_applied_anim_index = -1;
		_model_state->last_applied_anim_frame = -1;
	}

	void Entity::useSharedModel(const Model& model)
	{
		unloadModelData();

		if (model.meshCount == 0)
			return;

		_model_state->model = model;
		_model_state->model_loaded = true;
		_model_state->owns_model = false;
		_model_state->local_model_bounding_box = GetModelBoundingBox(_model_state->model);
		_model_state->local_model_bounding_box_valid = true;
	}

	bool Entity::alignLoadedModelToGround()
	{
		if (!_model_state->model_loaded)
			return false;

		const BoundingBox bounds = _model_state->local_model_bounding_box_valid
			? _model_state->local_model_bounding_box
			: GetModelBoundingBox(_model_state->model);
		_model_state->model.transform = MatrixMultiply(MatrixTranslate(0.0f, -bounds.min.y, 0.0f), _model_state->model.transform);
		return true;
	}

	void Entity::renderLoadedModel(const Color tint) const
	{
		if (!_model_state->model_loaded)
			return;

		DrawModelEx(
			_model_state->model,
			getWorldPos3D(),
			{0.0f, 1.0f, 0.0f},
			getRotation(),
			{getScale(), getScale(), getScale()},
			tint);
	}

	bool Entity::fitLoadedModelToHeight(const float target_height, const bool center_xz, const bool align_to_ground)
	{
		if (!_model_state->model_loaded || target_height <= 0.0f)
			return false;

		_model_state->local_model_bounding_box = GetModelBoundingBox(_model_state->model);
		_model_state->local_model_bounding_box_valid = true;
		const BoundingBox bounds = _model_state->local_model_bounding_box;
		const float model_height = bounds.max.y - bounds.min.y;
		if (model_height <= 1e-8f)
			return false;

		setScale(target_height / model_height);

		const float offset_x = center_xz ? -0.5f * (bounds.min.x + bounds.max.x) : 0.0f;
		const float offset_y = align_to_ground ? -bounds.min.y : 0.0f;
		const float offset_z = center_xz ? -0.5f * (bounds.min.z + bounds.max.z) : 0.0f;
		_model_state->model.transform = MatrixMultiply(MatrixTranslate(offset_x, offset_y, offset_z), _model_state->model.transform);
		return true;
	}

	Model& Entity::getModel()
	{
		return _model_state->model;
	}

	bool Entity::hasModelLoaded() const
	{
		return _model_state->model_loaded;
	}

} // namespace Nawia::Entity
