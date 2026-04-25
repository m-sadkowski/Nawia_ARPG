#pragma once
#include "Entity.h"
#include "Interactable.h"
#include "Collider.h"

namespace Nawia::Entity {
    class InteractiveTrigger : public Entity, public Interactable {
    public:
        using Entity::Entity;

        void onInteract(Entity& instigator) override {}
    };
}

