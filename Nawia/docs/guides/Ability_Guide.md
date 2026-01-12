# Przewodnik po klasie Ability

Klasa `Ability` (`src/entity/abilities/Ability.h`) jest bazą dla wszystkich umiejętności (skilli) w grze. Odpowiada za logikę "rzucania" czaru, zarządzanie cooldownem oraz statystykami.

## Czym jest Ability?
Ability to obiekt logiczny (nie jest Entity, nie renderuje się na mapie), który należy do `Entity` (castera).
Jego głównym zadaniem jest stworzenie efektu (np. pocisku) w momencie użycia.

## Jak stworzyć nową umiejętność?

Należy stworzyć klasę dziedziczącą po `Nawia::Entity::Ability`.

### Wymagania
1. Konstruktor przyjmujący tekstury (ikona, efekt) i ewentualnie statystyki.
2. Nadpisanie metody `cast`.
3. (Opcjonalnie) Nadpisanie `update`.

### Przykład: FireballAbility

#### Nagłówek (.h)
```cpp
#pragma once
#include "Ability.h"

namespace Nawia::Entity {

    class FireballAbility : public Ability {
    public:
        // Konstruktor przyjmuje teksturę dla pocisku (którą przekaże do effectu) i ikonę do UI
        FireballAbility(const std::shared_ptr<Texture2D>& projectile_tex, const std::shared_ptr<Texture2D>& icon_tex);

        // Metoda cast zwraca wskaźnik na nowy Entity (efekt/pocisk)
        std::unique_ptr<Entity> cast(float target_x, float target_y) override;

    private:
        std::shared_ptr<Texture2D> _projectile_tex;
    };

}
```

#### Implementacja (.cpp)
```cpp
#include "FireballAbility.h"
#include "AbilityEffect.h" // Poprawny include

namespace Nawia::Entity {

    FireballAbility::FireballAbility(const std::shared_ptr<Texture2D>& proj_tex, const std::shared_ptr<Texture2D>& icon_tex)
        : Ability(
            "Fireball", // Nazwa
            Ability::getStatsFromJson("fireball"), // Możesz pobrać staty z JSON lub wpisać ręcznie
            AbilityTargetType::POINT, // Typ celowania (POINT, UNIT, SELF)
            icon_tex // Ikona do UI
          ),
          _projectile_tex(proj_tex)
    {
        // Jeśli nie korzystasz z JSONa:
        // _stats.cooldown = 2.0f;
    }

    std::unique_ptr<Entity> FireballAbility::cast(float target_x, float target_y) {
        // Oblicz pozycję startową - użyj getCollider()->getCenter() jeśli to możliwe
        Vector2 startPos = _caster->getCollider() ? _caster->getCollider()->getCenter() : _caster->getPosition();
        
        // Stwórz efekt (pocisk). Klasa FireballEffect musi dziedziczyć po AbilityEffect (lub Entity).
        auto fireball = std::make_unique<FireballEffect>(
            startPos.x, startPos.y, 
            _projectile_tex, 
            _stats, // Przekazujemy statystyki (damage itp.)
            target_x, target_y // Cel
        );

        // WAŻNE: Ustaw faction castera, żeby pocisk nie ranił twórcy ani sojuszników!
        fireball->setFaction(_caster->getFaction());

        return fireball;
    }

}
```

## Szczegóły implementacji

### Statystyki (`AbilityStats`)
Struktura `AbilityStats` zawiera:
- `damage`: obrażenia
- `cooldown`: czas odnowienia (s)
- `projectile_speed`: prędkość pocisku
- `cast_range`: zasięg
- `hitbox_radius`: wielkość kolizji
- `duration`: czas trwania efektu

### Typy celowania (`AbilityTargetType`)
Enum `AbilityTargetType` określa, jak gracz używa umiejętności:
- `POINT`: Umiejętność celowana w dowolny punkt na ziemi (np. Fireball, Blink). Wymaga myszki.
- `UNIT`: Umiejętność wymagająca kliknięcia w konkretny cel (np. targeted stun).
- `SELF`: Umiejętność natychmiastowa na sobie (np. tarcza, leczenie w miejscu). Nie wymaga myszki.

### Metoda `cast(x, y)`
To najważniejsza metoda.
- **Wejście**: Koordynaty myszki/celu w świecie gry.
- **Wyjście**: `std::unique_ptr<Entity>`.
  - Jeśli umiejętność tworzy pocisk/efekt, zwróć go. System gry doda go do świata (`_pending_spawns`).
  - Jeśli umiejętność jest natychmiastowa i nie tworzy wizualnego bytu w świecie (np. leczenie), wykonaj logikę tutaj i zwróć `nullptr`.

## Co robi klasa bazowa?
Klasa `Ability` automatycznie obsługuje:
- **Cooldown**: `isReady()` zwróci fałsz, dopóki cooldown nie minie. Po wywołaniu `cast` (przez silnik), cooldown resetuje się sam (jeśli używasz standardowego flow).
- **Dostęp do Castera**: Masz wskaźnik `_caster` (Entity, który rzucił czar).

## Najczęstsze błędy
- Zapominanie o przekazaniu `_stats` do efektu (powoduje 0 dmg).
- Nie ustawianie `_faction` dla stworzonego pocisku (może ranić sojuszników).
- Zwracanie `nullptr` w `cast` kiedy powinien pojawić się efekt wizualny (nic się nie pojawi).
- Błędne obliczanie pozycji startowej (pocisk wylatuje z "nóg" zamiast ze środka postaci) -> Używaj `getCollider()->getCenter()`.
