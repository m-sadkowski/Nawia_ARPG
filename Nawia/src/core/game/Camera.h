#pragma once
#include "Constants.h"

#include <Entity.h>
#include <raylib.h>

namespace Nawia::Core
{

	/**
	 * @class GameCamera
	 * @brief Wraps Raylib Camera3D with isometric-like follow behavior.
	 *
	 * The camera looks down at an angle, following the player on the XZ plane.
	 * Entity positions use Vector2{x, y} which maps to world 3D as {x, 0, y}.
	 */
	struct GameCamera
	{
		Camera3D cam3d = {};
		float zoom = 1.0f;

		GameCamera()
		{
			cam3d.position = Vector3{ 0.0f, CAMERA_HEIGHT, CAMERA_DISTANCE };
			cam3d.target = Vector3{ 0.0f, 0.0f, 0.0f };
			cam3d.up = Vector3{ 0.0f, 1.0f, 0.0f };
			cam3d.fovy = CAMERA_FOV;
			cam3d.projection = CAMERA_PERSPECTIVE;
		}

		void handleInput()
		{
			float wheel = GetMouseWheelMove();
			if (wheel != 0.0f)
			{
				zoom -= wheel * 0.1f;
				if (zoom < 0.3f) zoom = 0.3f;
				if (zoom > 2.0f) zoom = 2.0f;
			}
		}

		void follow(const Entity::Entity* target)
		{
			if (!target) return;

			const float world_x = target->getX();
			const float world_z = target->getY(); // Entity Y maps to world Z

			cam3d.target = Vector3{ world_x, 0.0f, world_z };
			cam3d.position = Vector3{
				world_x - (CAMERA_DISTANCE * 0.7f),
				CAMERA_HEIGHT,
				world_z + (CAMERA_DISTANCE * 0.7f)
			};
			cam3d.fovy = CAMERA_FOV * zoom;
		}

		/// Get the underlying Camera3D for Raylib calls
		[[nodiscard]] const Camera3D& get() const { return cam3d; }
		[[nodiscard]] Camera3D& get() { return cam3d; }
	};

} // namespace Nawia::Core