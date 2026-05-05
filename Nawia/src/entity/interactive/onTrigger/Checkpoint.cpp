#include "Checkpoint.h"

#include <Collider.h>
#include <Engine.h>
#include <Player.h>

#include <iostream>

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

                // Informujemy QuestManager o dotarciu do checkpointu.
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
		// Checkpoint jest niewidoczny dla gracza, widoczny tylko w trybie diagnostycznym.
        if (DebugColliders && _collider) {
            auto* rect_collider = dynamic_cast<RectangleCollider*>(_collider.get());
            if (rect_collider) {
		// Rysujemy diagnostyczne pudełko 3D w pozycji kolidera.
                Vector2 center = rect_collider->getPosition();
                float w = rect_collider->getWidth();
                float h = rect_collider->getHeight();

                const float ground_height = getAltitude();
                Color fill_color = Color{0, 255, 0, 100};
                DrawCube(Vector3{center.x, ground_height + 0.1f, center.y}, w, 0.2f, h, fill_color);
                DrawCubeWires(Vector3{center.x, ground_height + 0.1f, center.y}, w, 0.2f, h, GREEN);
                
                if (_activated) {
                    // Tekst "SAVED" pojawia się nad checkpointem w przestrzeni ekranu.
                    Vector2 screen_pos = GetWorldToScreen(Vector3{center.x, ground_height + 0.5f, center.y}, camera);
                    DrawText("SAVED", (int)(screen_pos.x - 15), (int)(screen_pos.y - 10), 10, GREEN);
                }
            }
        }
    }

    float Checkpoint::getInteractionRange() {
        return 0;
    }
    
} // namespace Nawia::Entity
