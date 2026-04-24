# Przewodnik po klasie Enemy

Przeciwnicy w grze dziedziczą po klasie `Nawia::Entity::EnemyInterface`, która sama jest rozszerzeniem `Entity`.

## Struktura Przeciwnika

Enemy Interface zapewnia dostęp do obiektu mapy (`Core::Map*`), co jest kluczowe dla nawigacji i AI. W najnowszej wersji silnika postać 3D nie potrzebuje już własnego dedykowanego Collidera, gdyż system zderzeń obsługiwany jest autentycznie poprzez geometrię 3D (`Mesh Raycast`) oraz wirtualny system odpychania zaimplementowany w Menedżerze Entitiów.

### Podstawowy szkielet

```cpp
#include "EnemyInterface.h"

namespace Nawia::Entity {

    class Goblin : public EnemyInterface {
    public:
        Goblin(float x, float y, Core::Map* map)
            : EnemyInterface("Goblin", x, y, nullptr, 50, map) // 50 HP
        {
            // Ładowanie precyzyjnego modelu 3D
            loadModel("assets/models/goblin.glb");
            addAnimation("idle", "assets/animations/goblin_idle.glb");
            addAnimation("walk", "assets/animations/goblin_walk.glb");
            addAnimation("attack", "assets/animations/goblin_attack.glb");

            // Frakcja (ważne dla otrzymywania obrażeń oraz agresji AI)
            setFaction(Faction::Enemy);
            
            // Startowa animacja
            playAnimation("idle");
        }

        void update(float dt) override;
        
    private:
        enum class State { Idle, Chasing, Attacking };
        State _state = State::Idle;
        std::weak_ptr<Entity> _target; // Używaj weak_ptr dla targetowania, unika memory leaks!
    };
}
```

## Implementacja AI (Metoda Update)
W metodzie `update` należy zaimplementować maszynę stanów.

```cpp
void Goblin::update(float dt) {
    EnemyInterface::update(dt); // Obsługa fizyki i animacji bazowej

    // Prosta logika AI
    auto target = _target.lock();
    if (!target || target->isDead()) {
        // ...znajdź gracza poprzez pętlę na EntityManagerze...
        return;
    }

    // Używamy getCenter() dla wycelowania w korpus:
    Vector2 myPos = getCenter();
    Vector2 targetPos = target->getCenter();

    float dist = std::sqrt(std::pow(targetPos.x - myPos.x, 2) + std::pow(targetPos.y - myPos.y, 2));

    switch (_state) {
        case State::Idle:
            if (dist < 8.0f) { // Skale 3D są mniejsze (np. promienie operują na jednostkach rzędu 5-10)
                _state = State::Chasing;
                playAnimation("walk");
            }
            break;

        case State::Chasing:
            if (dist < 1.0f) { // Jesteśmy przy wrogu
                _state = State::Attacking;
                playAnimation("attack", false, true); // Lock movement blokuje chodzenie przy ataku
            } else {
                // Ruch w stronę gracza (patrzenie bazuje od stóp modelu)
                rotateTowards(target->getX(), target->getY());
                
                // Modyfikacja wektora logiki fizycznej
            }
            break;

        case State::Attacking:
            if (!isAnimationLocked()) { 
                // Animacja ataku się skończyła (lock movement puszcza)
                if (dist < 1.5f) {
                    target->takeDamage(10); // Dodanie ataku (najlepiej jako nowy Effect w świecie 3D)
                }
                
                _state = State::Idle;
                playAnimation("idle");
            }
            break;
    }
}
```

## Ważne Metody i Pola

### `_map`
Wskaźnik na `Core::Map`. Użyj go, aby sprawdzać walkability (czy płytka terenowa jest zdatna do przejścia).

### `takeDamage(int dmg)`
Działa standardowo jak w Entity. Przeciwnik zareaguje zgubieniem życia i sam uśmierci się odpalając animację po stracie punktów HP. Jeśli zechcesz wrzucić animację otrzymania trafienia, wywołaj tutaj `playAnimation(...)`.

### Fizyka (Pushing)
Twój wróg zostanie automatycznie oddalony z rejonu kolizji od każdego innego sojusznika i wroga. `EntityManager` gwarantuje zautomatyzowane przesuwanie walców przestrzennych zablokowanych o siebie przeciwników, więc po prostu każ swojemu goblinowi iść do ataku!

## Tekstury i Animacje
Używaj `loadModel` i `playAnimation`. 
- Pamiętaj o ustawieniu flagi `loop = false` dla animacji jednorazowych (atak, śmierć).
- Użyj `lock_movement = true` dla ataków, żeby postać nie "ślizgała się" podczas odgrywania ataku wręcz.

## Umieszczanie wrogów na mapie (JSON)

Wrogowie nie są już tworzeni ręcznie w konstruktorze levelu. Zamiast tego definiujesz ich w pliku JSON `assets/data/level_entities/<nazwa_levelu>.json`:

```json
{
    "location": "Demo Arena",
    "type": "devil",
    "name": "Demon Strażnik",
    "x": 10.0, "y": 15.0,
    "hp": 200,
    "trigger_radius": 15.0
}
```

Wróg startuje jako **dormant** (niewidoczny, zamrożony) i budzi się gdy gracz podejdzie na odległość `trigger_radius`. Aby dodać nowy typ wroga, zarejestruj go w `EntityFactory::create()`.

Szczegóły w `Level_Guide.md`.
