#include "BossWall.h"

#include <Collider.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

    BossWall::BossWall(float x, float y, float width, float height, Color color)
        : Entity("BossWall", x, y, nullptr, 1), _wall_color(color), _wall_width(width), _wall_height(height)
    {
        setType(EntityType::Wall);
        setFaction(Faction::None);
        setCollider(std::make_unique<RectangleCollider>(this, width, height, 0.0f, 0.0f));
    }

    void BossWall::update(float dt) {
        _pulse_timer += dt;
    }

    void BossWall::render(const Camera3D& camera) {
        auto* rect_collider = dynamic_cast<RectangleCollider*>(_collider.get());
        if (!rect_collider) return;

        Vector2 center = rect_collider->getPosition();
        float w = _wall_width;
        float h = _wall_height;

        float pulse = 0.5f + 0.5f * std::sin(_pulse_timer * 2.0f);
        unsigned char base_alpha = _wall_color.a;
        unsigned char pulsed_alpha = static_cast<unsigned char>(base_alpha * (0.6f + 0.4f * pulse));
        
        Color render_color = { _wall_color.r, _wall_color.g, _wall_color.b, pulsed_alpha };
        
        constexpr float wall_3d_height = 4.0f;

        DrawCube(Vector3{center.x, wall_3d_height / 2.0f, center.y}, w, wall_3d_height, h, render_color);

        Color wire_color = { 
            static_cast<unsigned char>(std::min(255, _wall_color.r + 60)),
            static_cast<unsigned char>(std::min(255, _wall_color.g + 20)),
            static_cast<unsigned char>(std::min(255, _wall_color.b + 20)),
            static_cast<unsigned char>(180 + static_cast<int>(75 * pulse))
        };
        DrawCubeWires(Vector3{center.x, wall_3d_height / 2.0f, center.y}, w, wall_3d_height, h, wire_color);
    }

} // namespace Nawia::Entity
