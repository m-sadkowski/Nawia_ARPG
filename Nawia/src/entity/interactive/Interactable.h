#pragma once
#include "Entity.h"
namespace Nawia::Entity {
    class Interactable {
    
    public:
        virtual ~Interactable() = default;

        
        virtual void onInteract(Entity& instigator) = 0;

       
        virtual void onTriggerEnter(Entity& other) = 0;

        
        virtual bool canInteract() const { return true; }

        virtual float getInteractionRange() = 0;
    };
}