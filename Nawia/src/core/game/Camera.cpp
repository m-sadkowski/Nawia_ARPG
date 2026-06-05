#include "Camera.h"

#include <Entity.h>

#include <algorithm>

namespace Nawia::Core {

	namespace {
		constexpr float k_zoom_step = 0.1f;
		constexpr float k_min_zoom_factor = 0.3f;
		constexpr float k_max_zoom_factor = 2.0f;
		constexpr float k_default_zoom_factor = 0.75f;
		constexpr float k_target_height_offset = 1.2f;
		constexpr float k_horizontal_follow_factor = 0.7f;
	}

	GameCamera::GameCamera() {
		_zoom_factor = k_default_zoom_factor;
		_camera_3d.position = Vector3{ 0.0f, CAMERA_HEIGHT, CAMERA_DISTANCE };
		_camera_3d.target = Vector3{ 0.0f, 0.0f, 0.0f };
		_camera_3d.up = Vector3{ 0.0f, 1.0f, 0.0f };
		_camera_3d.fovy = CAMERA_FOV * _zoom_factor;
		_camera_3d.projection = CAMERA_PERSPECTIVE;
	}

	void GameCamera::handleInput() {
		const float mouse_wheel_delta = GetMouseWheelMove();
		if (mouse_wheel_delta == 0.0f)
			return;

		_zoom_factor -= mouse_wheel_delta * k_zoom_step;
		_zoom_factor = std::clamp(_zoom_factor, k_min_zoom_factor, k_max_zoom_factor);
	}

	void GameCamera::resetZoom() {
		_zoom_factor = k_default_zoom_factor;
	}

	void GameCamera::resetZoom(const float zoom_factor) {
		_zoom_factor = std::clamp(zoom_factor, k_min_zoom_factor, k_max_zoom_factor);
	}

	void GameCamera::follow(const Entity::Entity* target, const float target_height_multiplier) {
		if (!target)
			return;

		Vector3 target_world_position = target->getWorldPos3D();
		target_world_position.y += k_target_height_offset * std::clamp(target_height_multiplier, 0.0f, 2.0f);

		_camera_3d.target = target_world_position;
		_camera_3d.position = Vector3{
			target_world_position.x - (CAMERA_DISTANCE * k_horizontal_follow_factor),
			target_world_position.y + CAMERA_HEIGHT,
			target_world_position.z + (CAMERA_DISTANCE * k_horizontal_follow_factor)
		};
		_camera_3d.fovy = CAMERA_FOV * _zoom_factor;
	}

	const Camera3D& GameCamera::get() const {
		return _camera_3d;
	}

	Camera3D& GameCamera::get() {
		return _camera_3d;
	}

} // namespace Nawia::Core
