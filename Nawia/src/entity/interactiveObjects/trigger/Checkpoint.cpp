#include "Checkpoint.h"
#include <Logger.h>
#include <SDL3/SDL.h>

namespace Nawia::Entity {

    Checkpoint::Checkpoint(float x, float y, const std::shared_ptr<SDL_Texture>& texture)
        : Trigger(x, y, texture, 100,5, 5), _activated(false)
    {
    }

    void Checkpoint::update(float delta_time) {
        
    }

    bool Checkpoint::isInteractive() const {
        
        return !_activated;
    }

    void Checkpoint::interaction() {
        if (isInteractive()) {
            _activated = true;

            Core::Logger::debugLog("Checkpoint: Punkt zapisu aktywowany!");

            
        }
    }

}