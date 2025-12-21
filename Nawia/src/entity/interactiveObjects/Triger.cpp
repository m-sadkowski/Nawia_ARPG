#include "Trigger.h"

namespace Nawia::Entity {

    Trigger::Trigger(float x, float y, const std::shared_ptr<SDL_Texture>& texture, int max_hp, float tw, float th)
        : InteractiveObject(x, y, texture, max_hp), _width(tw), _height(th){}

    bool Trigger::checkCollision(const std::shared_ptr<Entity>& target) const {
        if (!target) return false;

        // Pobieramy punkt docelowy (Gracza)
        float px = target->getX();
        float py = target->getY();

        // Wyliczamy granice Triggera na podstawie jego specyficznych wymiarów
        // Zak³adamy, ¿e getX() i getY() to œrodek obiektu
        float myLeft = getX() - (_width / 2.0f);
        float myRight = getX() + (_width / 2.0f);
        float myTop = getY() - (_height / 2.0f);
        float myBottom = getY() + (_height / 2.0f); // Tu by³ b³¹d (getX)

        // Sprawdzamy, czy punkt px, py zawiera siê wewn¹trz (Point-in-AABB)
        if (px >= myLeft && px <= myRight &&
            py >= myTop && py <= myBottom)
        {
            return true;
        }

        return false;
    }
}