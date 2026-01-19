#include "Checkpoint.h"
#include <iostream>

#include "Collider.h"
#include "InteractiveTrigger.h"
#include "Constants.h"

namespace Nawia::Entity {

    Checkpoint::Checkpoint(const std::string& name, float x, float y)
        : Nawia::Entity::InteractiveTrigger(name, x, y, nullptr, 1)
    {
        setFaction(Faction::None);
        _use_3d_rendering = false;
        setCollider(std::make_unique<RectangleCollider>(this, 2.0f, 1.0f, 0.0f, 0.0f));
    }

    void Checkpoint::onTriggerEnter(Entity& other) {
        if (!_activated && other.getFaction() == Faction::Player) {
            std::cout << "Checkpoint '" << _name << "' aktywowany przez " << other.getName() << "!" << std::endl;
            _activated = true;
        }
    }

    void Checkpoint::update(float delta_time) {
        Entity::update(delta_time);
    }

    void Checkpoint::render(float offset_x, float offset_y) {
        // Checkpoint is invisible to the player - visible only in debug mode
        if (DebugColliders && _collider) {
            auto* rect_collider = dynamic_cast<RectangleCollider*>(_collider.get());
            if (rect_collider) {
                // Draw isometric rhombus like map tiles
                float world_x = rect_collider->getPosition().x;
                float world_y = rect_collider->getPosition().y;
                float half_w = rect_collider->getWidth() / 2.0f;
                float half_h = rect_collider->getHeight() / 2.0f;
                
                // 4 corners of the collider in world space
                Vector2 top = getIsoPos(world_x - half_w, world_y - half_h, offset_x, offset_y);     // top
                Vector2 right = getIsoPos(world_x + half_w, world_y - half_h, offset_x, offset_y);   // right
                Vector2 bottom = getIsoPos(world_x + half_w, world_y + half_h, offset_x, offset_y);  // bottom
                Vector2 left = getIsoPos(world_x - half_w, world_y + half_h, offset_x, offset_y);    // left
                
                // Semi-transparent green fill - drawn as 2 triangles
                Color fill_color = Color{0, 255, 0, 100};
                DrawTriangle(top, left, bottom, fill_color);
                DrawTriangle(top, bottom, right, fill_color);
                
                // Isometric rhombus border
                DrawLineV(top, right, GREEN);
                DrawLineV(right, bottom, GREEN);
                DrawLineV(bottom, left, GREEN);
                DrawLineV(left, top, GREEN);
                
                if (_activated) {
                    Vector2 center_pt = getIsoPos(world_x, world_y, offset_x, offset_y);
                    DrawText("SAVED", (int)(center_pt.x - 15), (int)(center_pt.y - 10), 10, GREEN);
                }
            }
        }
        // In normal mode we render nothing - checkpoint is invisible
    }

    float Checkpoint::getInteractionRange() {
        return 0;
    }
    
} // namespace Nawia::Entity
