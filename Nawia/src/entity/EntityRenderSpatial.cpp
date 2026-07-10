#include "Entity.h"

#include <Collider.h>
#include <EntityModelState.h>
#include <EntityMovementState.h>
#include <EntityVisualState.h>

#include <raymath.h>

#include <algorithm>
#include <limits>

namespace Nawia::Entity {

	void Entity::render(const Camera3D& camera)
	{
		if (_dormant)
			return;

		if (_model_state->model_loaded)
		{
			const Vector3 pos3d = getWorldPos3D();
			const float visual_rotation = _movement_state->rotation + _visual_state->modelFacingOffset();
			auto draw_model = [this, pos3d, visual_rotation](const Color tint) {
				if (!_visual_state->hasHiddenMeshes()) {
					DrawModelEx(
						_model_state->model,
						pos3d,
						{0.0f, 1.0f, 0.0f},
						visual_rotation,
						{_movement_state->scale, _movement_state->scale, _movement_state->scale},
						tint);
					return;
				}

				Model model = _model_state->model;
				const Matrix mat_scale = MatrixScale(_movement_state->scale, _movement_state->scale, _movement_state->scale);
				const Matrix mat_rotation = MatrixRotate({0.0f, 1.0f, 0.0f}, visual_rotation * DEG2RAD);
				const Matrix mat_translation = MatrixTranslate(pos3d.x, pos3d.y, pos3d.z);
				model.transform = MatrixMultiply(model.transform, MatrixMultiply(MatrixMultiply(mat_scale, mat_rotation), mat_translation));

				for (int mesh_index = 0; mesh_index < model.meshCount; ++mesh_index) {
					if (_visual_state->isMeshHidden(mesh_index))
						continue;

					const int material_index = model.meshMaterial ? model.meshMaterial[mesh_index] : 0;
					Color original = model.materials[material_index].maps[MATERIAL_MAP_DIFFUSE].color;
					Color color_tint = {
						static_cast<unsigned char>((static_cast<int>(original.r) * static_cast<int>(tint.r)) / 255),
						static_cast<unsigned char>((static_cast<int>(original.g) * static_cast<int>(tint.g)) / 255),
						static_cast<unsigned char>((static_cast<int>(original.b) * static_cast<int>(tint.b)) / 255),
						static_cast<unsigned char>((static_cast<int>(original.a) * static_cast<int>(tint.a)) / 255)
					};
					model.materials[material_index].maps[MATERIAL_MAP_DIFFUSE].color = color_tint;
					DrawMesh(model.meshes[mesh_index], model.materials[material_index], model.transform);
					model.materials[material_index].maps[MATERIAL_MAP_DIFFUSE].color = original;
				}
			};

			draw_model(_visual_state->modelTint());
			drawAttachedModel(pos3d, visual_rotation);

			if (_visual_state->hovered() && _type != EntityType::Player)
				draw_model(Fade(BLACK, 0.2f));
		}

		if (DebugColliders)
		{
			if (_collider)
				_collider->render(camera);

			if (_model_state->model_loaded)
				DrawBoundingBox(getBoundingBox(), GREEN);
		}
	}

	bool Entity::isMouseOver(const float screen_x, const float screen_y, const Camera3D& camera) const
	{
		if (_model_state->model_loaded)
		{
			const Ray mouse_ray = GetScreenToWorldRay(Vector2{screen_x, screen_y}, camera);
			if (!GetRayCollisionBox(mouse_ray, getBoundingBox()).hit)
				return false;

			return checkRayHitsMesh(mouse_ray);
		}

		const Vector2 screen_position = getScreenPosition(camera);
		constexpr float click_radius = 30.0f;
		const float dx = screen_x - screen_position.x;
		const float dy = screen_y - screen_position.y;
		return (dx * dx + dy * dy) < (click_radius * click_radius);
	}

	Matrix Entity::getWorldTransformMatrix() const
	{
		const Vector3 pos3d = getWorldPos3D();
		const Matrix mat_translate = MatrixTranslate(pos3d.x, pos3d.y, pos3d.z);
		const float visual_rotation = (_movement_state->rotation + _visual_state->modelFacingOffset()) * DEG2RAD;
		const Matrix mat_rotate = MatrixRotate({0.0f, 1.0f, 0.0f}, visual_rotation);
		const Matrix mat_scale = MatrixScale(_movement_state->scale, _movement_state->scale, _movement_state->scale);
		return MatrixMultiply(
			MatrixMultiply(MatrixMultiply(mat_scale, _model_state->model.transform), mat_rotate),
			mat_translate);
	}

	bool Entity::checkRayHitsMesh(const Ray& ray) const
	{
		if (!_model_state->model_loaded)
			return false;
		if (!GetRayCollisionBox(ray, getBoundingBox()).hit)
			return false;

		const Matrix world_transform = getWorldTransformMatrix();
		for (int i = 0; i < _model_state->model.meshCount; i++)
		{
			const RayCollision collision = GetRayCollisionMesh(ray, _model_state->model.meshes[i], world_transform);
			if (collision.hit)
				return true;
		}
		return false;
	}

	RayCollision Entity::getRayMeshCollision(const Ray& ray) const
	{
		RayCollision closest = {};
		closest.hit = false;
		closest.distance = 1e30f;

		if (!_model_state->model_loaded)
			return closest;
		if (!GetRayCollisionBox(ray, getBoundingBox()).hit)
			return closest;

		const Matrix world_transform = getWorldTransformMatrix();
		for (int i = 0; i < _model_state->model.meshCount; i++)
		{
			const RayCollision collision = GetRayCollisionMesh(ray, _model_state->model.meshes[i], world_transform);
			if (collision.hit && collision.distance < closest.distance)
				closest = collision;
		}
		return closest;
	}

	bool Entity::isVisibleInCamera(const Camera3D& camera, const float screen_margin) const
	{
		if (_dormant)
			return false;

		if (!_model_state->model_loaded)
			return DebugColliders;

		const int screen_width = GetScreenWidth();
		const int screen_height = GetScreenHeight();
		if (screen_width <= 0 || screen_height <= 0)
			return true;

		const auto is_projected_on_screen = [&](const Vector3 point) {
			const Vector2 projected = GetWorldToScreen(point, camera);
			return projected.x >= -screen_margin &&
				projected.x <= static_cast<float>(screen_width) + screen_margin &&
				projected.y >= -screen_margin &&
				projected.y <= static_cast<float>(screen_height) + screen_margin;
		};

		if (is_projected_on_screen(getWorldPos3D()))
			return true;

		const BoundingBox box = getBoundingBox();
		const Vector3 corners[8] = {
			{box.min.x, box.min.y, box.min.z},
			{box.max.x, box.min.y, box.min.z},
			{box.min.x, box.max.y, box.min.z},
			{box.max.x, box.max.y, box.min.z},
			{box.min.x, box.min.y, box.max.z},
			{box.max.x, box.min.y, box.max.z},
			{box.min.x, box.max.y, box.max.z},
			{box.max.x, box.max.y, box.max.z}
		};

		for (const Vector3& corner : corners) {
			if (is_projected_on_screen(corner))
				return true;
		}

		const Vector3 center = {
			(box.min.x + box.max.x) * 0.5f,
			(box.min.y + box.max.y) * 0.5f,
			(box.min.z + box.max.z) * 0.5f
		};
		return is_projected_on_screen(center);
	}

	BoundingBox Entity::getBoundingBox() const
	{
		if (!_model_state->model_loaded)
		{
			const Vector3 pos = getWorldPos3D();
			return BoundingBox{
				Vector3{pos.x - 0.5f, pos.y, pos.z - 0.5f},
				Vector3{pos.x + 0.5f, pos.y + 1.0f, pos.z + 0.5f}
			};
		}

		const BoundingBox local_bb = _model_state->local_model_bounding_box_valid
			? _model_state->local_model_bounding_box
			: GetModelBoundingBox(_model_state->model);
		const Matrix world_transform = getWorldTransformMatrix();
		const Vector3 corners[] = {
			{local_bb.min.x, local_bb.min.y, local_bb.min.z},
			{local_bb.min.x, local_bb.min.y, local_bb.max.z},
			{local_bb.min.x, local_bb.max.y, local_bb.min.z},
			{local_bb.min.x, local_bb.max.y, local_bb.max.z},
			{local_bb.max.x, local_bb.min.y, local_bb.min.z},
			{local_bb.max.x, local_bb.min.y, local_bb.max.z},
			{local_bb.max.x, local_bb.max.y, local_bb.min.z},
			{local_bb.max.x, local_bb.max.y, local_bb.max.z},
		};

		BoundingBox world_box = {
			Vector3{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
			Vector3{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()}
		};

		for (const Vector3& corner : corners) {
			const Vector3 transformed = Vector3Transform(corner, world_transform);
			world_box.min = Vector3Min(world_box.min, transformed);
			world_box.max = Vector3Max(world_box.max, transformed);
		}

		return world_box;
	}

	Vector2 Entity::getScreenPosition(const Camera3D& camera) const
	{
		const Vector3 world_position = {_movement_state->position.x, 0.0f, _movement_state->position.y};
		return GetWorldToScreen(world_position, camera);
	}

	Vector2 Entity::getCenter() const
	{
		return _movement_state->position;
	}

} // namespace Nawia::Entity
