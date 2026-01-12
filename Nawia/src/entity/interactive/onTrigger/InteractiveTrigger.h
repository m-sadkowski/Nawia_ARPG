#pragma once
#include "Entity.h"
#include "Interactable.h"
#include "Collider.h"

namespace Nawia::Entity {
    class InteractiveTrigger : public Entity, public Interactable {
   
    public:
        using Entity::Entity;

        // Pusta implementacja interakcji, bo przez te obiekty siê tylko przechodzi
        void onInteract(Entity& instigator) override {}

        // onTriggerEnter pozostaje czysto wirtualne do zrobienia w np. Checkpoint.cpp

        
    };
}
