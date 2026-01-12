# Przewodnik po klasie Enemy

Przeciwnicy w grze dziedziczą po klasie `Nawia::Entity::EnemyInterface`, która sama jest rozszerzeniem `Entity`.

## Struktura Przeciwnika

Enemy Interface zapewnia dostęp do obiektu mapy (`Core::Map*`), co jest kluczowe dla nawigacji i AI.

### Podstawowy szkielet

```cpp
#include "EnemyInterface.h"
#include "Collider.h" // Poprawny include

namespace Nawia::Entity {

    class Goblin : public EnemyInterface {
    public:
        Goblin(float x, float y, Core::Map* map)
            : EnemyInterface("Goblin", x, y, nullptr, 50, map) // 50 HP
        {
            // Ładowanie modelu
            loadModel("assets/models/goblin.glb");
            addAnimation("idle", "assets/animations/goblin_idle.glb");
            addAnimation("walk", "assets/animations/goblin_walk.glb");
            addAnimation("attack", "assets/animations/goblin_attack.glb");
            
            // Collider
            // Ustawiamy lekki offset w górę (-10), żeby collider obejmował tułów/nogi, a nie tylko stopy.
            // Sprawdź w trybie debug (Entity::DebugColliders = true) czy pasuje!
            setCollider(std::make_unique<CircleCollider>(this, 15.0f, 0.0f, -10.0f));

            // Frakcja (ważne!)
            setFaction(Faction::Enemy);
            
            // Startowa animacja
            playAnimation("idle");
        }

        void update(float dt) override;
        
    private:
        enum class State { Idle, Chasing, Attacking };
        State _state = State::Idle;
        std::shared_ptr<Entity> _target; // Gracz
    };
}
```

## Implementacja AI (Metoda Update)
W metodzie `update` należy zaimplementować maszynę stanów.

```cpp
void Goblin::update(float dt) {
    EnemyInterface::update(dt); // Obsługa fizyki i animacji

    // Przykład prostej logiki
    if (!_target) {
        // ...znajdź gracza...
        return;
    }

    // Używamy getCenter() dla precyzji:
    Vector2 myPos = getCollider() ? getCollider()->getCenter() : _pos;
    Vector2 targetPos = _target->getCollider() ? _target->getCollider()->getCenter() : _target->getPosition();

    float dist = Vector2Distance(myPos, targetPos);

    switch (_state) {
        case State::Idle:
            if (dist < 300.0f) {
                _state = State::Chasing;
                playAnimation("walk");
            }
            break;

        case State::Chasing:
            if (dist < 50.0f) {
                _state = State::Attacking;
                playAnimation("attack", false, true); // Lock movement
                setVelocity(0, 0); // Zatrzymaj się
            } else {
                // Ruch w stronę gracza
                rotateTowards(_target->getX(), _target->getY());
                
                // Prosty ruch (bez pathfindingu)
                Vector2 dir = Vector2Normalize(Vector2Subtract(targetPos, myPos));
                setVelocity(dir.x * 100.0f, dir.y * 100.0f);
            }
            break;

        case State::Attacking:
            if (!isAnimationLocked()) { 
                // Animacja ataku się skończyła (lock movement puszcza)
                if (dist < 60.0f) _target->takeDamage(10);
                
                _state = State::Idle;
                playAnimation("idle");
            }
            break;
    }
}
```

## Ważne Metody i Pola

### `_map`
Wskaźnik na `Core::Map`. Użyj go, aby sprawdzać kolizje ze ścianami lub szukać ścieżki.

### `takeDamage(int dmg)`
Działa standardowo jak w Entity. Możesz nadpisać, aby dodać logikę aggro (zacznij gonić tego, kto cię uderzył) lub efektów (krew, odrzut).

### `setCollider`
Pamiętaj o colliderze, inaczej gracz będzie przenikał przez przeciwnika, a ataki nie będą wchodzić.

## Tekstury i Animacje
Używaj `loadModel` i `playAnimation`. 
- Pamiętaj o ustawieniu flagi `loop = false` dla animacji jednorazowych (atak, śmierć).
- Użyj `lock_movement = true` dla ataków, żeby postać nie "ślizgała się" podczas machania mieczem.
