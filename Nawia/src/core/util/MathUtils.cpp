#include "MathUtils.h"

#include <raymath.h>

#include <cmath>

namespace Nawia::Core {

	Vector2 screenToWorld(const Camera3D& camera, const float screen_x, const float screen_y) {
		const Ray ray = GetScreenToWorldRay({screen_x, screen_y}, camera);

		if (std::abs(ray.direction.y) < 0.0001f)
			return {camera.target.x, camera.target.z};

		const float distance_to_ground = -ray.position.y / ray.direction.y;

		if (distance_to_ground < 0.0f)
			return {camera.target.x, camera.target.z};

		const float world_x = ray.position.x + distance_to_ground * ray.direction.x;
		const float world_z = ray.position.z + distance_to_ground * ray.direction.z;

		return {world_x, world_z};
	}

} // namespace Nawia::Core
