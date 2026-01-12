# Przewodnik po klasie Entity

Klasa `Entity` (`src/entity/Entity.h`) jest bazową klasą dla wszystkich obiektów w grze Nawia ARPG. Reprezentuje ona każdy byt, który posiada pozycję w świecie, może być wyświetlany, posiadać punkty życia oraz wchodzić w interakcje fizyczne.

## Czym jest Entity?
Entity to nie tylko statyczny obiekt. To kontener na:
- **Transformację**: Pozycja (`_pos`), prędkość (`_velocity`), skala (`_scale`), rotacja (`_rotation`).
- **Grafikę**: Tekstura 2D lub Model 3D z animacjami.
- **Fizykę**: Collider (`_collider`) służący do detekcji kolizji.
- **Logikę gry**: Punkty życia (`_hp`), frakcja (`_faction`), umiejętności (`_abilities`).

## Jak zaimplementować własny obiekt?

Aby stworzyć nowy obiekt, należy stworzyć klasę dziedziczącą publicznie po `Nawia::Entity::Entity`.

### Krok 1: Nagłówek (.h)
```cpp
#pragma once
#include "Entity.h"

namespace Nawia::Entity {

    class MyCustomEntity : public Entity {
    public:
        // Konstruktor
        MyCustomEntity(float x, float y, const std::shared_ptr<Texture2D>& texture);

        // Nadpisane metody
        void update(float dt) override;
        void render(float offset_x, float offset_y) override; // Opcjonalne, jeśli standardowy render wystarcza
        void takeDamage(int dmg) override; // Opcjonalne

    private:
        float _timer = 0.0f;
    };

}
```

### Krok 2: Implementacja (.cpp)
```cpp
#include "MyCustomEntity.h"
#include "Collider.h" // Poprawny include (bez podkatalogów)

namespace Nawia::Entity {

    MyCustomEntity::MyCustomEntity(float x, float y, const std::shared_ptr<Texture2D>& texture)
        : Entity("MyEntity", x, y, texture, 100) // 100 to Max HP
    {
        // ------------------------------------------------------------------
        // KONFIGURACJA COLLIDERA
        // ------------------------------------------------------------------
        // Collider jest kluczowy nie tylko dla kolizji, ale też dla hitboxów.
        // Często tekstury mają dużo pustego, przezroczystego miejsca naokoło postaci.
        // Dlatego NIE NALEŻY polegać na środku tekstury, ale ustawić collider z offsetem.
        
        float radius = 20.0f;
        float offsetX = 0.0f; 
        float offsetY = -10.0f; // Przesunięcie w górę, żeby collider był np. na nogach lub tułowiu
        
        setCollider(std::make_unique<CircleCollider>(this, radius, offsetX, offsetY));
        
        // WAŻNE: Od teraz, jeśli chcesz pobrać "prawdziwy" środek bytu (np. żeby wycelować w niego pocisk),
        // używaj: getCollider()->getCenter().
        // Metody getX()/getY() zwracają pozycję "zakotwiczenia" entity (zazwyczaj pod nogami/środek sprita),
        // ale to collider definiuje "bryłę" w którą się trafia.
        
        // ------------------------------------------------------------------
        // FRAKCJA
        // ------------------------------------------------------------------
        // Aby system walki wiedział, kogo można atakować, ustaw frakcję.
        setFaction(Faction::Enemy); // Inne opcje: Player, Neutral, Ally, None
    }

    void MyCustomEntity::update(float dt) {
        // Wywołaj bazowe update (obsługuje animacje, fizykę ruchu, cooldowny skilli)
        Entity::update(dt);

        // Twoja logika
        _timer += dt;
        if (_timer > 5.0f) {
            // Zrób coś co 5 sekund
            _timer = 0.0f;
        }
    }

    void MyCustomEntity::render(float offset_x, float offset_y) {
        // Jeśli chcesz narysować standardową teksturę/model:
        Entity::render(offset_x, offset_y);

        // Tutaj można dodać np. pasek życia rysowany nad postacią
    }

}
```

## System Fizyki i Colliderów

### Dlaczego offsety są ważne?
W grach izometrycznych/top-down, sprite postaci często obejmuje też przestrzeń nad głową. Jeśli ustawisz collider na środku obrazka, będzie on wisiał w powietrzu lub obejmował pustą przestrzeń.
W konstruktorze collidera (Circle, Rectangle, Cone) ostatnie dwa parametry to `offset_x` i `offset_y` względem pozycji Entity (`_pos`).

**Przykład:** Postać stoi na ziemi w punkcie (X, Y). Tekstura jest wysoka. Chcemy, aby kolizja (np. z przeszkodami) dotyczyła jej stóp.
```cpp
// Promień 15, przesunięcie Y o 0 (jeśli punkt zaczepienia to stopy) lub dopasowane eksperymentalnie.
setCollider(std::make_unique<CircleCollider>(this, 15.0f, 0.0f, 0.0f));
```

### Pobieranie pozycji: `getCollider()->getCenter()` czy `getX()`?
- `getX() / getY()`: Zwraca surową pozycję entity w świecie. To punkt, w którym rysowana jest tekstura (z uwzględnieniem offsetu rysowania).
- `getCollider()->getCenter()`: **ZALECANE** do logiki gry. Zwraca rzeczywisty środek bryły kolizyjnej.
    - Jeśli strzelasz fireballa: celuj w `target->getCollider()->getCenter()`.
    - Jeśli sprawdzasz odległość AI: sprawdzaj dystans do `center`.

### Typy Colliderów (`Collider.h`)
1. **CircleCollider**: `(Entity* owner, float radius, float offX, float offY)`
   - Najlepszy dla postaci i pocisków. Szybki w obliczeniach.
2. **RectangleCollider**: `(Entity* owner, float w, float h, float offX, float offY)`
   - Dla skrzyń, ścian, drzwi.
3. **ConeCollider**: `(Entity* owner, float radius, float angle, float offX, float offY)`
   - Specjalistyczny, np. do wykrywania trafień obszarowych w stożku przed postacią.

### Debugowanie
Nie musisz zgadywać offsetów. Włącz tryb debugowania, aby zobaczyć obrysy colliderów (zielone linie).
Aby włączyć globalne rysowanie colliderów, ustaw statyczną flagę:
```cpp
// Np. w Game.cpp lub na przycisk
Nawia::Entity::Entity::DebugColliders = true;
```

## Frakcje (Faction System)

Każde Entity posiada pole `_faction` (enum `Faction`).
```cpp
enum class Faction {
    Player,
    Enemy,
    Neutral,
    Ally,
    None
};
```
Ustawienie frakcji (`setFaction(...)`) jest niezbędne, aby:
1. Pociski wiedziały, kogo ranić (Fireball rzucony przez Enemy nie rani Enemy).
2. AI wiedziało, kogo gonić.
3. System targetowania (myszką) wiedział, kogo podświetlić jako wroga.

## System Graficzny: 2D vs 3D

Entity wspiera dwa tryby wyświetlania.

### 1. Tryb 2D (Sprite)
Domyślny tryb. Wymaga podania `std::shared_ptr<Texture2D>` w konstruktorze. Obiekt będzie rysowany jako sprite w rzucie izometrycznym.

### 2. Tryb 3D (Modele i Animacje)
Aby używać modelu 3D:
1. W konstruktorze przekaż teksturę (może być używana jako fallback lub UI), lub `nullptr`.
2. Załaduj model metodą `loadModel`.
3. Dodaj animacje metodą `addAnimation`.

```cpp
// W konstruktorze:
loadModel("assets/models/character.glb"); // Ładuje model
addAnimation("idle", "assets/animations/idle.glb");
addAnimation("run", "assets/animations/run.glb");

// Uruchom animację startową
playAnimation("idle");
```

**Ważne o animacjach:**
- `playAnimation(name, loop, lock_movement)`:
    - `loop`: czy animacja ma się pętlalć (np. bieg).
    - `lock_movement`: czy zablokować ruch postaci na czas trwania animacji (np. atak).

## Metody do nadpisania (Override)

| Metoda | Opis | Czy wywoływać `Base::Metoda`? |
| --- | --- | --- |
| `update(float dt)` | Główna pętla logiki. | **TAK**, na początku. Obsługuje ruch i animacje. |
| `render(float ox, float oy)` | Rysowanie na ekranie. | **TAK**, jeśli chcesz narysować bazowy model/teksturę. **NIE**, jeśli chcesz rysować coś zupełnie własnego. |
| `takeDamage(int dmg)` | Reakcja na obrażenia. | **TAK**, odejmuje HP i sprawdza śmierć. Dodaj tu np. dźwięk oberwania. |
| `isMouseOver(...)` | Sprawdza czy myszka jest nad obiektem. | Zazwyczaj nie trzeba nadpisywać, bazowa wersja poprawnie używa collidera. |

## Przydatne Metody (API)

- `move(float x, float y)` / `setVelocity(vx, vy)`: Poruszanie postacią. Bazowa klasa używa `_velocity` w `update`.
- `rotateTowards(x, y)`: Obraca postać (model 3D) w stronę wybranego punktu w świecie.
- `playAnimation(...)`: Sterowanie animacjami.
- `addAbility(...)`: Dodawanie umiejętności do listy dostępnych.
- `die()`: Zabija enta (HP = 0), zazwyczaj wywołuje animację śmierci lub usuwa obiekt.

## Cykl Życia
1. **Konstruktor**: Inicjalizacja zasobów, collidera, modelu.
2. **Update**: Co klatkę. Logika, AI, ruch.
3. **Render**: Co klatkę (po update). Rysowanie.
4. **Destruktor**: Sprzątanie zasobów.
