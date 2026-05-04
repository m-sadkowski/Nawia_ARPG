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
	class GameCamera
	{
	public:
		GameCamera();

		void handleInput();
		void follow(const Entity::Entity* target);

		/// Get the underlying Camera3D for Raylib calls.
		[[nodiscard]] const Camera3D& get() const;
		[[nodiscard]] Camera3D& get();

	private:
		Camera3D _camera_3d = {};
		float _zoom_factor = 1.0f;
	};

} // namespace Nawia::Core
