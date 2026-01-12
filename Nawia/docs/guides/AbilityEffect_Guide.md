# Przewodnik po klasie AbilityEffect

Klasa `AbilityEffect` (`src/entity/abilities/AbilityEffect.h`) to wyspecjalizowana klasa `Entity` reprezentująca efekty umiejętności, takie jak pociski, eksplozje, czy obszary działania (AoE).

## Czym różni się od zwykłego Entity?
- Posiada `AbilityStats` (obrażenia, czas trwania).
- Posiada wbudowaną obsługę czasu życia (`Lifetime`). zniknie sam po upływie `duration`.
- Posiada metody do obsługi kolizji specyficzne dla walki (`onCollision`, `checkCollision`).

## Jak stworzyć efekt (np. Pocisk)?

Tworzymy klasę dziedziczącą po `Nawia::Entity::AbilityEffect` (lub bezpośrednio po `Entity`, jeśli nie potrzebujemy mechanik AbilityEffect, ale zalecane jest użycie dedykowanej bazy).

### Krok 1: Implementacja

```cpp
#include "AbilityEffect.h"
#include "Collider.h" // Poprawny include

namespace Nawia::Entity {

    class FireballEffect : public AbilityEffect {
    public:
        FireballEffect(float startX, float startY, const std::shared_ptr<Texture2D>& tex, const AbilityStats& stats, float targetX, float targetY)
            : AbilityEffect("FireballEffect", startX, startY, tex, stats)
        {
            // Oblicz wektor prędkości w stronę celu
            float angle = atan2(targetY - startY, targetX - startX);
            float speed = stats.projectile_speed;
            
            setVelocity(cos(angle) * speed, sin(angle) * speed);

            // Ustaw collider
            // Pociski są małe, zazwyczaj koło bez offsetu wystarcza. promień bierzemy z stats.hitbox_radius.
            setCollider(std::make_unique<CircleCollider>(this, stats.hitbox_radius, 0.0f, 0.0f));
        }

        // Możesz nadpisać update, jeśli potrzebujesz specjalnego zachowania (np. naprowadzanie)
        void update(float dt) override {
            AbilityEffect::update(dt); // WAŻNE: Obsługuje ruch i czas życia (_timer)
        }

        // Obsługa trafienia
        void onCollision(const std::shared_ptr<Entity>& target) override {
            // Klasa bazowa sprawdza czy już trafiliśmy ten cel (żeby nie zadawać dmg co klatkę)
            if (hasHit(target)) return;
            
            // Frakcje są zazwyczaj filtrowane w checkCollision, ale dla pewności:
            if (target->getFaction() == getFaction()) return;

            // Zadaj obrażenia
            target->takeDamage(_stats.damage);
            addHit(target); // Zapisz, że ten cel już dostał

            // Jeśli to pocisk "jednorazowy" (znika po trafieniu):
            // die(); 
            // lub stwórz efekt wybuchu i dopiero zgiń.
        }
    };
}
```

## Kluczowe Metody

### `checkCollision(target)`
Domyślna implementacja w `AbilityEffect` używa colliderów obu obiektów oraz sprawdza frakcje (ignoruje sojuszników).
Nadpisz tę metodę, jeśli potrzebujesz niestandardowej logiki (np. hitscan, ignorowanie ścian).

### `onCollision(target)`
Wywoływana przez silnik, gdy wykryta zostanie kolizja z **wrogim** bytem (zwróconym przez `checkCollision`).
Tutaj implementujesz logikę: zadanie obrażeń, nałożenie statusu (slow/stun), zniszczenie pocisku.

### `update(dt)`
Bazowa metoda `AbilityEffect::update`:
1. Aktualizuje pozycję na podstawie velocity.
2. Zlicza `_timer`.
3. Jeśli `_timer > _stats.duration`, oznacza obiekt jako martwy (`die()`).

## System Kolizji i Frakcji (Szczegóły)
- Pamiętaj, aby ustawić Frakcję pocisku taką samą jak Castera (`setFaction`).
- `AbilityEffect` filtruje kolizje sojusznicze. Fireball gracza nie trafi gracza ani innego gracza (w coop), trafil tylko wroga (`Faction::Enemy`).

## Tekstury i Animacje
Działają tak samo jak w `Entity`.
- **2D**: Przekaż teksturę w konstruktorze.
- **3D**: Użyj `loadModel` w ciele konstruktora.
- **Particles**: Dla efektów cząsteczkowych (np. dym, ogień) możesz stworzyć osobne klasy particles, lub użyć prostych animowanych Entity spawnowanych przy `die()`.
