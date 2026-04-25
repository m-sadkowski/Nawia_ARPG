#include "Checkpoint.h"
#include <iostream>

#include "Collider.h"
#include "InteractiveTrigger.h"
#include <Player.h>
#include <Engine.h>

namespace Nawia::Entity {

    Checkpoint::Checkpoint(const std::string& name, float x, float y)
        : Nawia::Entity::InteractiveTrigger(name, x, y, nullptr, 1)
    {
        setFaction(Faction::None);
        setCollider(std::make_unique<RectangleCollider>(this, 2.0f, 1.0f, 0.0f, 0.0f));
    }

    void Checkpoint::onTriggerEnter(Entity& other) {
        if (!_activated && other.getFaction() == Faction::Player) {
            std::cout << "Checkpoint '" << _name << "' aktywowany przez " << other.getName() << "!" << std::endl;
            _activated = true;
            if (auto* player = dynamic_cast<Player*>(&other)) {
                player->setRespawnPoint(this->getCenter());

                // Notify QuestManager about checkpoint reached
                if (player->getEngine()) {
                    player->getEngine()->getQuestManager().notifyCheckpointReached(getName());
                }
            }
        }
    }

    void Checkpoint::update(float delta_time) {
        Entity::update(delta_time);
    }

    void Checkpoint::render(const Camera3D& camera) {
        // Checkpoint is invisible to the player - visible only in debug mode
        if (DebugColliders && _collider) {
            auto* rect_collider = dynamic_cast<RectangleCollider*>(_collider.get());
            if (rect_collider) {
                // Draw 3D debug box at collider position
                Vector2 center = rect_collider->getPosition();
                float w = rect_collider->getWidth();
                float h = rect_collider->getHeight();

                Color fill_color = Color{0, 255, 0, 100};
                DrawCube(Vector3{center.x, 0.1f, center.y}, w, 0.2f, h, fill_color);
                DrawCubeWires(Vector3{center.x, 0.1f, center.y}, w, 0.2f, h, GREEN);
                
                if (_activated) {
                    // Draw "SAVED" text above the checkpoint in screen space
                    Vector2 screen_pos = GetWorldToScreen(Vector3{center.x, 0.5f, center.y}, camera);
                    DrawText("SAVED", (int)(screen_pos.x - 15), (int)(screen_pos.y - 10), 10, GREEN);
                }
            }
        }
    }

    float Checkpoint::getInteractionRange() {
        return 0;
    }
    
} // namespace Nawia::Entity
