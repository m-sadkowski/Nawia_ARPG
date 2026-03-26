#pragma once

#include <raylib.h>

namespace Nawia::Core {

	/**
	 * @brief Convert screen coordinates to world position on the ground plane (Y=0).
	 * Uses ray-casting from the camera through the screen point onto the Y=0 plane.
	 * @param camera The 3D camera used for rendering
	 * @param screen_x Mouse X position on screen
	 * @param screen_y Mouse Y position on screen
	 * @return Vector2 world position {x, z} on the ground plane
	 */
	Vector2 screenToWorld(const Camera3D& camera, float screen_x, float screen_y);

} // namespace Nawia::Core