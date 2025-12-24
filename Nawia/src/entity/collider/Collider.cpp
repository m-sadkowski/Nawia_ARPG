#include "Collider.h"
#include "Entity.h"

#include <MathUtils.h>
#include <Constants.h>

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

        // normalize angle difference to -180..180 range
        float angle_diff = angle_to_circle - cone_rotation;
        while (angle_diff > 180.0f) angle_diff -= 360.0f;
        while (angle_diff < -180.0f) angle_diff += 360.0f;

        // check if angle is within the cone's field of view
        return std::abs(angle_diff) <= cone_angle / 2.0f;
    }

    // check collision between cone and rectangle
    bool checkConeRect(const Vector2 cone_pos, const float cone_radius, const float cone_angle, const float cone_rotation, const Rectangle rect) {
        // approximate rectangle as a circle for simpler cone check
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
                const float rot = 0.0f; // placeholder
                return checkConeCircle(other_cone->getPosition(), other_cone->getRadius(), other_cone->getAngle(), rot, my_pos, _radius);
            }
            default: return false;
        }
    }

    void CircleCollider::render(const float offset_x, const float offset_y) const {
    	Core::Point2D screen_pt = _owner->getScreenPos(_owner->getX() + _offset.x, _owner->getY() + _offset.y, offset_x, offset_y);
        const auto screen_pos = Vector2{ screen_pt.getX(), screen_pt.getY() };

        DrawCircleLines(static_cast<int>(screen_pos.x), static_cast<int>(screen_pos.y), _radius * 32.0f, RED);
        DrawCircleLines(static_cast<int>(screen_pos.x), static_cast<int>(screen_pos.y), _radius * Core::TILE_WIDTH / 2.0f, RED);
    }

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
                const float rot = 0.0f; // placeholder
                return checkConeRect(other_cone->getPosition(), other_cone->getRadius(), other_cone->getAngle(), rot, my_rect);
            }
            default: return false;
         }
    }

    void RectangleCollider::render(const float offset_x, const float offset_y) const {
        Core::Point2D center_pt = _owner->getScreenPos(getPosition().x, getPosition().y, offset_x, offset_y);
        const auto center = Vector2{ center_pt.getX(), center_pt.getY()};
        
        DrawCircle(static_cast<int>(center.x), static_cast<int>(center.y), 2, BLUE);
        
        const Vector2 pos = getPosition();
        const Vector2 corners[4] = {
            { pos.x - _width / 2, pos.y - _height / 2 },
            { pos.x + _width / 2, pos.y - _height / 2 },
            { pos.x + _width / 2, pos.y + _height / 2 },
            { pos.x - _width / 2, pos.y + _height / 2 }
        };

        // project and draw the rectangle corners
        Vector2 screen_corners[4];
        for(int i = 0; i < 4; ++i) {
        	Core::Point2D p = _owner->getScreenPos(corners[i].x, corners[i].y, offset_x, offset_y);
             screen_corners[i] = Vector2{ p.getX(), p.getY() };
        }
        for(int i = 0; i < 4; ++i) {
            DrawLineV(screen_corners[i], screen_corners[(i + 1) % 4], BLUE);
        }
    }

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

    void ConeCollider::render(const float offset_x, const float offset_y) const {
        const Vector2 tip_world = getPosition();
        
        const float rot_deg = _owner->getRotation();
        const float angle_half = _angle / 2.0f;
        
        // calculate endpoints in world space
        const float rad_left = (rot_deg - angle_half) * DEG2RAD;
        const float rad_right = (rot_deg + angle_half) * DEG2RAD;
        
        const Vector2 end_left_world = {
             tip_world.x + cos(rad_left) * _radius,
             tip_world.y + sin(rad_left) * _radius
        };
        
        const Vector2 end_right_world = {
             tip_world.x + cos(rad_right) * _radius,
             tip_world.y + sin(rad_right) * _radius
        };
        
        // project to screen space
    	Core::Point2D tip_screen_pt = _owner->getScreenPos(tip_world.x, tip_world.y, offset_x, offset_y);
    	Core::Point2D left_screen_pt = _owner->getScreenPos(end_left_world.x, end_left_world.y, offset_x, offset_y);
    	Core::Point2D right_screen_pt = _owner->getScreenPos(end_right_world.x, end_right_world.y, offset_x, offset_y);

    	const Vector2 tip_screen = { tip_screen_pt.getX(), tip_screen_pt.getY() };
        const Vector2 left_screen = { left_screen_pt.getX(), left_screen_pt.getY() };
        const Vector2 right_screen = { right_screen_pt.getX(), right_screen_pt.getY() };
        
        // draw lines
        DrawLineV(tip_screen, left_screen, GREEN);
        DrawLineV(tip_screen, right_screen, GREEN);
        DrawLineV(left_screen, right_screen, GREEN); // arc approximation
    }


} // namespace Nawia::Entity
