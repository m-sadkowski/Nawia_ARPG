#pragma once
#include "Entity.h"

namespace Nawia::Entity {

    /**
     * @interface Interactable
     * @brief Interface for objects the player can interact with.
     * 
     * Implement this interface for any entity that should respond to player
     * interaction (clicking) or trigger events (walking into).
     * 
     * ## Types of Interactive Objects
     * 
     * - **Clickable objects** (chests, NPCs, levers): Use `InteractiveClickable` base class
     *   and implement `onInteract()` for click behavior.
     * 
     * - **Trigger zones** (traps, teleports, checkpoints): Implement `onTriggerEnter()`
     *   for collision-based activation.
     * 
     * ## Key Methods
     * 
     * - `onInteract()`: Called when player clicks and is in range
     * - `onTriggerEnter()`: Called when an entity enters the interaction zone
     * - `canInteract()`: Override to add conditions (e.g., locked doors)
     * - `getInteractionRange()`: Maximum distance for interaction
     * 
     * @note Objects need a collider for interaction detection!
     */
    class Interactable {
    
    public:
        virtual ~Interactable() = default;

        /**
         * @brief Called when an entity interacts with this object (e.g., player clicks).
         * @param instigator The entity that triggered the interaction
         */
        virtual void onInteract(Entity& instigator) = 0;

        /**
         * @brief Called when an entity enters this object's trigger zone.
         * @param other The entity that entered
         */
        virtual void onTriggerEnter(Entity& other) = 0;

        /**
         * @brief Check if interaction is currently possible.
         * Override to add conditions (e.g., cooldowns, locked state).
         * @return true if can be interacted with
         */
        virtual bool canInteract() const { return true; }

        /**
         * @brief Get the maximum interaction range.
         * @return Distance in world units
         */
        virtual float getInteractionRange() = 0;
    };
}