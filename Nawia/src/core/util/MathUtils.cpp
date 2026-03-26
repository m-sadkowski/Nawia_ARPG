#include "MathUtils.h"
#include <raymath.h>
#include <cmath>

namespace Nawia::Core {

    Vector2 screenToWorld(const Camera3D& camera, const float screen_x, const float screen_y)
    {
        // Get a ray from the screen point through the camera
        const Ray ray = GetScreenToWorldRay({screen_x, screen_y}, camera);

        // Intersect with the ground plane Y = 0
        // ray.position + t * ray.direction, solve for y = 0:
        // ray.position.y + t * ray.direction.y = 0
        // t = -ray.position.y / ray.direction.y

        if (std::abs(ray.direction.y) < 0.0001f)
        {
            // Ray is parallel to the ground plane, return camera target projection
            return { camera.target.x, camera.target.z };
        }

        const float t = -ray.position.y / ray.direction.y;

        if (t < 0.0f)
        {
            // Intersection is behind the camera
            return { camera.target.x, camera.target.z };
        }

        const float world_x = ray.position.x + t * ray.direction.x;
        const float world_z = ray.position.z + t * ray.direction.z;

        return { world_x, world_z };
    }

} // namespace Nawia::Core
