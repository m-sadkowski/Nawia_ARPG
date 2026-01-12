#pragma once
#include "Entity.h"
#include "Interactable.h"

namespace Nawia::Entity{
    class InteractiveClickable : public Entity, public Interactable {
        
        
    public:
        using Entity::Entity;

        // Pusta implementacja triggera, bo w te obiekty siê klika
        void onTriggerEnter(Nawia::Entity::Entity& other) override {}

        // onInteract pozostaje czysto wirtualne do zrobienia w np. Chest.cpp
    };
	
}

