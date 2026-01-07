#include "Checkpoint.h"
#include <iostream>

#include "Collider.h"
#include "InteractiveTrigger.h"

namespace Nawia::Entity {

    Checkpoint::Checkpoint(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture)
        : Nawia::Entity::InteractiveTrigger(name, x, y, texture, 1)
    {
        setFaction(Faction::None);
        _use_3d_rendering = false;
        setCollider(std::make_unique<RectangleCollider>(this, 2.0f, 1.0f, 0.0f, 0.0f));
    
    }

    void Checkpoint::onTriggerEnter(Entity& other) {
        
        if (!_activated && other.getFaction() == Faction::Player) {
            std::cout << "Checkpoint '" << _name << "' aktywowany przez " << other.getName() << "!" << std::endl;

            _activated = true;

            // Opcjonalnie: zmiana koloru lub animacja po aktywacji
            // playAnimation("activated", false);
        }
    }

    void Checkpoint::update(float delta_time) {
        Entity::update(delta_time);
    }

    void Checkpoint::render(float offset_x, float offset_y) {
        // Jeœli checkpoint jest aktywny, mo¿emy go narysowaæ inaczej (np. na zielono)
        if (_activated) {
            // Logika specyficzna dla aktywnego checkpointu
            Entity::render(offset_x, offset_y);
            DrawText("SAVED", (int)(_pos.x - offset_x), (int)(_pos.y - offset_y - 20), 10, GREEN);
        }
        else {
            Entity::render(offset_x, offset_y);
        }
    }
    
} // namespace Nawia::Entity