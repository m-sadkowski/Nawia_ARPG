#include "Chest.h"
#include <Logger.h>
#include "InteractiveObject.h"
namespace Nawia::Entity {

    Chest::Chest(float x, float y, const std::shared_ptr<SDL_Texture>& texture)
        : Usable (x, y, texture, 10000), _isOpened(false)
    {
    }

    void Chest::update(float delta_time) {
        // Tu mo¿na dodaæ np. logikê animacji otwierania
    }

    bool Chest::isInteractive() const {
        return !_isOpened;
    }

    void Chest::interaction() {


        ///TEMPORARY  tutaj trzeba miec juz zrobione eq playera zeby cos z tym dzia³ac

        if (isInteractive()) {


            _isOpened = true;
            Core::Logger::debugLog("Chest: Interacted and opened!");
            Core::Logger::debugLog("You can take whatever you want!");

            

            const SDL_MessageBoxData data = {
            SDL_MESSAGEBOX_INFORMATION,
            nullptr,                
            "Zawartosc Skrzynki",   
            "Znalazles: Miecz Przeznaczenia oraz 50 zlota!\n\n(Kliknij OK, aby zamknac)", 
            0,
            nullptr,
            nullptr
            };

            SDL_ShowMessageBox(&data, nullptr);


            
           
        }
    }

}