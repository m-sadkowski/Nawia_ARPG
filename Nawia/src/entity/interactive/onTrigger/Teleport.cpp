#include "Teleport.h"

#include <Collider.h>
#include <Engine.h>
#include <Level.h>
#include <LevelManager.h>
#include <Logger.h>
#include <Player.h>

namespace Nawia::Entity {

    Teleport::Teleport(const std::string& name, float x, float y, Core::Engine* engine, const std::string& target_location)
        : InteractiveTrigger(name, x, y, nullptr, 1), _engine(engine), _target_location(target_location)
    {
        setFaction(Faction::None);
        setCollider(std::make_unique<RectangleCollider>(this, 2.0f, 1.0f, 0.0f, 0.0f));
    }

    void Teleport::onTriggerEnter(Entity& other) {
        if (isDormant()) return;

        if (other.getFaction() == Faction::Player) {
            Core::Logger::debugLog("Teleporting player to: " + _target_location);
            
            if (_engine) {
                auto* current_level = _engine->getLevelManager().getCurrentLevel();
                if (current_level)
                    current_level->changeLocation(_engine, _target_location);
            }
        }
    }

    void Teleport::update(float delta_time) {
        Entity::update(delta_time);
    }

    void Teleport::render(const Camera3D& camera) {
		// Teleport jest niewidoczny w normalnej grze, widoczny tylko w trybie diagnostycznym.
        if (DebugColliders && _collider) {
            auto* rect_collider = dynamic_cast<RectangleCollider*>(_collider.get());
            if (rect_collider) {
                Vector2 center = rect_collider->getPosition();
                float w = rect_collider->getWidth();
                float h = rect_collider->getHeight();

                const float ground_height = getAltitude();
                Color fill_color = Color{100, 0, 255, 100}; // Fiolet oznacza teleport.
                DrawCube(Vector3{center.x, ground_height + 0.1f, center.y}, w, 0.2f, h, fill_color);
                DrawCubeWires(Vector3{center.x, ground_height + 0.1f, center.y}, w, 0.2f, h, PURPLE);
                
                Vector2 screen_pos = GetWorldToScreen(Vector3{center.x, ground_height + 0.5f, center.y}, camera);
                DrawText(("TO " + _target_location).c_str(), (int)(screen_pos.x - 20), (int)(screen_pos.y - 10), 10, PURPLE);
            }
        }
    }

    float Teleport::getInteractionRange() {
        return 0.0f;
    }

} // namespace Nawia::Entity
