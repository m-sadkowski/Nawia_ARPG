#include "Teleport.h"

#include <Collider.h>
#include <Engine.h>
#include <Level.h>
#include <LevelManager.h>
#include <Logger.h>
#include <Player.h>
#include <BossManager.h>

namespace Nawia::Entity {

    Teleport::Teleport(const std::string& name, float x, float y, Core::Engine* engine, const std::string& target_location)
        : InteractiveTrigger(name, x, y, nullptr, 1), _engine(engine), _target_location(target_location)
    {
        _type = EntityType::Trigger;
        setFaction(Faction::None);
        loadModel("assets/models/fireball.glb");
        setScale(0.45f);
        setModelFacingOffset(0.0f);

        if (_model_loaded) {
            for (int i = 0; i < _model.materialCount; ++i) {
                _model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = Color{45, 20, 80, 255};
            }
        }

        setCollider(std::make_unique<RectangleCollider>(this, 2.0f, 1.0f, 0.0f, 0.0f));
    }

    void Teleport::onTriggerEnter(Entity& other) {
        if (isDormant()) return;

        if (other.getFaction() == Faction::Player) {
            if (_engine && _engine->getBossManager().isFightActive()) {
                Core::Logger::debugLog("Teleport zablokowany - trwa walka z bossem!");
                return;
            }

            Core::Logger::debugLog("Teleporting player to: " + _target_location);
            
            if (_engine) {
                auto* current_level = _engine->getLevelManager().getCurrentLevel();
                if (current_level)
                    current_level->changeLocation(_engine, _target_location);
            }
        }
    }

    void Teleport::update(float delta_time) {
        setRotation(getRotation() + 45.0f * delta_time);
        Entity::update(delta_time);
    }

    void Teleport::render(const Camera3D& camera) {
        if (!isDormant() && _model_loaded) {
            const Vector3 pos3d = getWorldPos3D();
            DrawModelEx(
                _model,
                {pos3d.x, pos3d.y + 0.35f, pos3d.z},
                {0.0f, 1.0f, 0.0f},
                getRotation(),
                {_scale, _scale, _scale},
                Color{35, 18, 70, 235});
        }

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
