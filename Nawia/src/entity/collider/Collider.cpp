#include "Collider.h"
#include "Entity.h"

#include <MathUtils.h>

#include <raymath.h>
#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

    // check collision between two circles
    bool checkCircleCircle(const Vector2 pos_1, const float r_1, const Vector2 pos_2, const float r_2) {
        return CheckCollisionCircles(pos_1, r_1, pos_2, r_2);
    }

    // check collision between circle and rectangle
    bool checkCircleRect(const Vector2 circle_pos, const float radius, const Rectangle rect) {
        return CheckCollisionCircleRec(circle_pos, radius, rect);
    }

    // check collision between two rectangles
    bool checkRectRect(const Rectangle rect_1, const Rectangle rect_2) {
        return CheckCollisionRecs(rect_1, rect_2);
    }

    // check collision between cone and circle
    bool checkConeCircle(const Vector2 cone_pos, const float cone_radius, const float cone_angle, const float cone_rotation, const Vector2 circle_pos, const float circle_radius) {
        const float dist_sq = Vector2DistanceSqr(cone_pos, circle_pos);
        const float max_dist = cone_radius + circle_radius;
        
        if (dist_sq > max_dist * max_dist) 
            return false;

        const Vector2 dir_to_circle = Vector2Subtract(circle_pos, cone_pos);
        const float angle_to_circle = std::atan2(dir_to_circle.y, dir_to_circle.x) * RAD2DEG;

        float angle_diff = angle_to_circle - cone_rotation;
        while (angle_diff > 180.0f) angle_diff -= 360.0f;
        while (angle_diff < -180.0f) angle_diff += 360.0f;

        return std::abs(angle_diff) <= cone_angle / 2.0f;
    }

    // check collision between cone and rectangle
    bool checkConeRect(const Vector2 cone_pos, const float cone_radius, const float cone_angle, const float cone_rotation, const Rectangle rect) {
        const float rect_radius = std::sqrt(rect.width * rect.width + rect.height * rect.height) / 2.0f;
        const Vector2 rect_center = { rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f };
        return checkConeCircle(cone_pos, cone_radius, cone_angle, cone_rotation, rect_center, rect_radius);
    }

    Vector2 Collider::getPosition() const {
        if (_owner) {
            return { _owner->getX() + _offset.x, _owner->getY() + _offset.y };
        }
        return _offset;
    }

    // =========================================================================
    // CircleCollider
    // =========================================================================

    bool CircleCollider::checkCollision(const Collider* other) const {
        if (!other) return false;

        const Vector2 my_pos = getPosition();

        switch (other->getType()) {
            case ColliderType::CIRCLE: {
                const auto* other_circle = dynamic_cast<const CircleCollider*>(other);
                return checkCircleCircle(my_pos, _radius, other_circle->getPosition(), other_circle->getRadius());
            }
            case ColliderType::RECTANGLE: {
                const auto* other_rect = dynamic_cast<const RectangleCollider*>(other);
                return checkCircleRect(my_pos, _radius, other_rect->getRect());
            }
            case ColliderType::CONE: {
                const auto* other_cone = dynamic_cast<const ConeCollider*>(other);
                const float rot = 0.0f;
                return checkConeCircle(other_cone->getPosition(), other_cone->getRadius(), other_cone->getAngle(), rot, my_pos, _radius);
            }
            default: return false;
        }
    }

    bool CircleCollider::checkCollision(const BoundingBox& target_box) const {
        Rectangle target_rect = { target_box.min.x, target_box.min.z, target_box.max.x - target_box.min.x, target_box.max.z - target_box.min.z };
        return checkCircleRect(getPosition(), _radius, target_rect);
    }

    bool CircleCollider::checkMeshCollision(const Entity* target) const {
        if (!target) return false;
        
        Vector3 origin = { getPosition().x, 1.0f, getPosition().y };
        Vector3 target_pos = { target->getX(), 1.0f, target->getY() };
        
        Vector3 dir = Vector3Normalize(Vector3Subtract(target_pos, origin));
        Ray ray = { origin, dir };
        
        RayCollision hit = target->getRayMeshCollision(ray);
        if (hit.hit && hit.distance <= _radius) {
            return true;
        }
        
        // Fast fallback if centers are very close (inside)
        float distSq = Vector2DistanceSqr(getPosition(), {target->getX(), target->getY()});
        if (distSq < _radius * _radius) return true;
        
        return false;
    }

    void CircleCollider::render(const Camera3D& camera) const {
        const Vector2 pos = getPosition();
        // Draw circle on ground plane (Y = 0.01 to avoid z-fighting)
        DrawCircle3D(
            Vector3{ pos.x, 0.01f, pos.y },
            _radius,
            Vector3{ 1.0f, 0.0f, 0.0f }, 90.0f,
            RED
        );
    }

    bool CircleCollider::checkPoint(float screen_x, float screen_y, const Camera3D& camera) const {
        // Convert screen click to world position on ground plane
        const Vector2 world_click = Core::screenToWorld(camera, screen_x, screen_y);
        const Vector2 center = getPosition();
        const float dx = world_click.x - center.x;
        const float dy = world_click.y - center.y;
        return (dx * dx + dy * dy) <= (_radius * _radius);
    }

    // =========================================================================
    // RectangleCollider
    // =========================================================================

    Rectangle RectangleCollider::getRect() const {
        const Vector2 pos = getPosition();
        return { pos.x - _width / 2.0f, pos.y - _height / 2.0f, _width, _height };
    }

    bool RectangleCollider::checkCollision(const Collider* other) const {
         if (!other) return false;
         
         const Vector2 my_pos = getPosition();
         const Rectangle my_rect = getRect();

         switch (other->getType()) {
            case ColliderType::CIRCLE: {
                const auto* other_circle = dynamic_cast<const CircleCollider*>(other);
                return checkCircleRect(other_circle->getPosition(), other_circle->getRadius(), my_rect);
            }
            case ColliderType::RECTANGLE: {
                 const auto* other_rect = dynamic_cast<const RectangleCollider*>(other);
                 return checkRectRect(my_rect, other_rect->getRect());
            }
            case ColliderType::CONE: {
                const auto* other_cone = dynamic_cast<const ConeCollider*>(other);
                const float rot = 0.0f;
                return checkConeRect(other_cone->getPosition(), other_cone->getRadius(), other_cone->getAngle(), rot, my_rect);
            }
            default: return false;
         }
    }

    bool RectangleCollider::checkCollision(const BoundingBox& target_box) const {
        Rectangle target_rect = { target_box.min.x, target_box.min.z, target_box.max.x - target_box.min.x, target_box.max.z - target_box.min.z };
        return checkRectRect(getRect(), target_rect);
    }

    bool RectangleCollider::checkMeshCollision(const Entity* target) const {
        if (!target) return false;
        
        Vector3 origin = { getPosition().x, 1.0f, getPosition().y };
        Vector3 target_pos = { target->getX(), 1.0f, target->getY() };
        
        Vector3 dir = Vector3Normalize(Vector3Subtract(target_pos, origin));
        Ray ray = { origin, dir };
        
        RayCollision hit = target->getRayMeshCollision(ray);
        if (hit.hit) {
            Rectangle rect = getRect();
            return CheckCollisionPointRec({hit.point.x, hit.point.z}, rect);
        }
        
        float max_dim = std::fmax(_width, _height);
        float distSq = Vector2DistanceSqr(getPosition(), {target->getX(), target->getY()});
        if (distSq < (max_dim/2) * (max_dim/2)) return true;
        
        return false;
    }

    void RectangleCollider::render(const Camera3D& camera) const {
        const Vector2 center = getPosition();
        // Draw a wireframe cube on the ground plane to represent the rectangle
        DrawCubeWires(
            Vector3{ center.x, 0.5f, center.y },
            _width, 1.0f, _height,
            BLUE
        );
        // Center mark
        DrawSphere(Vector3{ center.x, 0.01f, center.y }, 0.05f, RED);
    }

    bool RectangleCollider::checkPoint(float screen_x, float screen_y, const Camera3D& camera) const {
        const Vector2 world_click = Core::screenToWorld(camera, screen_x, screen_y);
        const Rectangle rect = getRect();
        return CheckCollisionPointRec(world_click, rect);
    }

    // =========================================================================
    // ConeCollider
    // =========================================================================

    bool ConeCollider::checkCollision(const Collider* other) const {
         if (!other) return false;
         
         const float rot = _owner->getRotation();
         const Vector2 my_pos = getPosition();

         switch (other->getType()) {
            case ColliderType::CIRCLE: {
                const auto* other_circle = dynamic_cast<const CircleCollider*>(other);
                return checkConeCircle(my_pos, _radius, _angle, rot, other_circle->getPosition(), other_circle->getRadius());
            }
            case ColliderType::RECTANGLE: {
                 const auto* other_rect = dynamic_cast<const RectangleCollider*>(other);
                 return checkConeRect(my_pos, _radius, _angle, rot, other_rect->getRect());
            }
             case ColliderType::CONE: {
                 return false;
             }
            default: return false;
         }
    }

    bool ConeCollider::checkCollision(const BoundingBox& target_box) const {
        const float rot = _owner->getRotation();
        Rectangle target_rect = { target_box.min.x, target_box.min.z, target_box.max.x - target_box.min.x, target_box.max.z - target_box.min.z };
        return checkConeRect(getPosition(), _radius, _angle, rot, target_rect);
    }

    bool ConeCollider::checkMeshCollision(const Entity* target) const {
        if (!target) return false;
        
        const float rot_deg = _owner->getRotation();
        const float angle_half = _angle / 2.0f;
        const int num_rays_h = 10; // Sweep 11 rays across the cone angle for highly accurate sweeping
        const int num_rays_v = 5; // Sweep 15 heights to catch enemies of different heights
        
        const float min_h = 0.1f;
        const float max_h = 2.0f;
        
        for (int j = 0; j < num_rays_v; j++) {
            float fraction_v = (num_rays_v == 1) ? 0.5f : (float)j / (num_rays_v - 1);
            float height = min_h + (max_h - min_h) * fraction_v;
            Vector3 origin = { getPosition().x, height, getPosition().y };
            
            for (int i = 0; i < num_rays_h; i++) {
                float fraction_h = (num_rays_h == 1) ? 0.5f : (float)i / (num_rays_h - 1);
                float current_angle_deg = (rot_deg - angle_half) + fraction_h * _angle;
                float rad = current_angle_deg * DEG2RAD;
                
                Vector3 dir = { std::cos(rad), 0.0f, std::sin(rad) };
                Ray ray = { origin, dir };
                
                RayCollision hit = target->getRayMeshCollision(ray);
                if (hit.hit && hit.distance <= _radius) {
                    return true;
                }
            }
        }
        
        return false;
    }

    void ConeCollider::render(const Camera3D& camera) const {
        const Vector2 tip_world = getPosition();
        
        const float rot_deg = _owner->getRotation();
        const float angle_half = _angle / 2.0f;
        
        const float rad_left = (rot_deg - angle_half) * DEG2RAD;
        const float rad_right = (rot_deg + angle_half) * DEG2RAD;
        
        const Vector3 tip_3d = { tip_world.x, 0.05f, tip_world.y };
        
        const Vector3 end_left_3d = {
            tip_world.x + cos(rad_left) * _radius,
            0.05f,
            tip_world.y + sin(rad_left) * _radius
        };
        
        const Vector3 end_right_3d = {
            tip_world.x + cos(rad_right) * _radius,
            0.05f,
            tip_world.y + sin(rad_right) * _radius
        };
        
        DrawLine3D(tip_3d, end_left_3d, GREEN);
        DrawLine3D(tip_3d, end_right_3d, GREEN);
        DrawLine3D(end_left_3d, end_right_3d, GREEN);

        // Render rays at multiple heights for visual volume
        const int num_rays_h = 10;
        const int num_rays_v = 5;
        const float min_h = 0.1f;
        const float max_h = 2.0f;

        for (int j = 0; j < num_rays_v; j++) {
            float fraction_v = (num_rays_v == 1) ? 0.5f : (float)j / (num_rays_v - 1);
            float height = min_h + (max_h - min_h) * fraction_v;
            Vector3 origin = { tip_world.x, height, tip_world.y };

            for (int i = 0; i < num_rays_h; i++) {
                float fraction_h = (num_rays_h == 1) ? 0.5f : (float)i / (num_rays_h - 1);
                float current_angle_deg = (rot_deg - angle_half) + fraction_h * _angle;
                float rad = current_angle_deg * DEG2RAD;

                Vector3 end = {
                    origin.x + std::cos(rad) * _radius,
                    height,
                    origin.z + std::sin(rad) * _radius
                };
                DrawLine3D(origin, end, ColorAlpha(GREEN, 0.3f));
            }
        }

        // debug center line
        const float rad_center = rot_deg * DEG2RAD;
        const Vector3 end_center_3d = {
            tip_world.x + cos(rad_center) * _radius,
            0.05f,
            tip_world.y + sin(rad_center) * _radius
        };
        DrawLine3D(tip_3d, end_center_3d, YELLOW);
    }

    bool ConeCollider::checkPoint(float screen_x, float screen_y, const Camera3D& camera) const {
        const Vector2 world_click = Core::screenToWorld(camera, screen_x, screen_y);
        const Vector2 tip_world = getPosition();

        const float rot_deg = _owner->getRotation();
        const float angle_half = _angle / 2.0f;

        const float rad_left = (rot_deg - angle_half) * DEG2RAD;
        const float rad_right = (rot_deg + angle_half) * DEG2RAD;

        const Vector2 end_left = {
             tip_world.x + cos(rad_left) * _radius,
             tip_world.y + sin(rad_left) * _radius
        };

        const Vector2 end_right = {
             tip_world.x + cos(rad_right) * _radius,
             tip_world.y + sin(rad_right) * _radius
        };

        return CheckCollisionPointTriangle(world_click, tip_world, end_left, end_right);
    }


} // namespace Nawia::Entity
