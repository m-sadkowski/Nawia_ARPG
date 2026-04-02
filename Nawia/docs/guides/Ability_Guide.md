# Przewodnik po klasie Ability

Klasa `Ability` (`src/entity/abilities/Ability.h`) jest bazą dla wszystkich umiejętności (skilli) w grze. Odpowiada za logikę "rzucania" czaru, zarządzanie czasem oczekiwania (cooldownem) oraz pobieraniem statystyk.

## Czym jest Ability?
Ability to wirtualny obiekt logiczny (nie jest dziedziczonym Entity, samo z siebie z racji braku fizyki nie renderuje się na mapie), który należy do `Entity` (np. postać rzucająca - caster).
Jego głównym zadaniem jest wykreowanie i rzut strefowym zjawiskiem w postaci np. obiekto-pocisku (Effect) w momencie użycia, który ukaże to na silniku wizualnym.

## Jak stworzyć nową umiejętność?

Należy stworzyć klasę dziedziczącą po `Nawia::Entity::Ability`.

### Przykład: SuperSlashAbility

#### Nagłówek (.h)
```cpp
#pragma once
#include "Ability.h"

namespace Nawia::Entity {

    class SuperSlashAbility : public Ability {
    public:
        // Konstruktor podpinający tekstury wizuali do HUD i przekazywane dla efektu przestrzeni 
        SuperSlashAbility(const std::shared_ptr<Texture2D>& slash_tex, const std::shared_ptr<Texture2D>& icon_tex);

        // Metoda cast zwraca wskaźnik na nowo urodzony obiekt przestrzenny zniszczenia Entity (efekt/cięcie)
        std::unique_ptr<Entity> cast(float target_x, float target_y) override;

    private:
        std::shared_ptr<Texture2D> _slash_texture;
    };

}
```

#### Implementacja (.cpp)
```cpp
#include "SuperSlashAbility.h"
#include "SwordSlashEffect.h" 

namespace Nawia::Entity {

    SuperSlashAbility::SuperSlashAbility(const std::shared_ptr<Texture2D>& slash_tex, const std::shared_ptr<Texture2D>& icon_tex)
        : Ability(
            "SwordSlash", // Nazwa ułatwiająca szukanie w paczkach JSON statsów
            Entity::getAbilityStatsFromJson("SwordSlash"), // Pobiera surowe uderzenie i obszar!
            AbilityTargetType::POINT, 
            icon_tex // Do GUI
          ),
          _slash_texture(slash_tex)
    {
    }

    std::unique_ptr<Entity> SuperSlashAbility::cast(float target_x, float target_y) {
        
        // 1. Obróć fizyczne ciało postaci w stronę rzutu myszką! Współrzędne to punkty głębi z Raylib.
        _caster->rotateTowardsCenter(target_x, target_y);
        
        // 2. Zapobiegaj sztywnemu chodzeniu przy cięciu z animacją blokowania ciała
        startCooldown();
        _caster->playAnimation("attack", false, true);

        // Pobierz tors swojego twórcy za fizyczny punkt spawnu obrażeń
        Vector2 startPos = _caster->getCenter();
        
        // 3. Stwórz sam fizyczny byt obszarowy 2D/3D (Zasięg + Kolizja siatki)
        auto slash = std::make_unique<SwordSlashEffect>(
            startPos.x, startPos.y,
            -_caster->getRotation(),  // kąt musi idealnie odzwierciedlać skręcenie korpusu postaci względem kamery!
            _slash_texture, 
            _stats
        );

        // WAŻNE: Dodaj frakcję, żeby ogień maga nie zranił maga ani jego przyjaciół.
        slash->setFaction(_caster->getFaction());

        return slash;
    }

}
```

## Szczegóły implementacji systemu Umiejętności (Ability)

### Statystyki (`AbilityStats`)
Struktura danych definiująca jak mocny jest skill pobierana z plikow `assets/data/abilities.json`:
- `damage`: ilość zabieranego HP
- `cooldown`: sekundy oczekiwania pomiędzy klatkami ataków.
- `cast_range`: ułamek do weryfikacji poza który nie możesz odpalić spacji (zasięg ataku)
- `hitbox_radius`: rozmiar Collidera nadawanego przy rzucie parametrami
- `duration`: czas zanim animacja lub Entity czaru zostaną usunięte.

### Typy celowania (`AbilityTargetType`)
Opis zachowań kontrolerów:
- `POINT`: Cięcie celowane w punkt z ziemi gdzie najeżdża kursor. Powszechne!
- `UNIT`: Precyzyjne wybranie namierzonego Entity (Stun / Dot)
- `SELF`: Automatycznie upuszczane na _castera (Tarcze)

### Ostrzeżenia
Błędne obliczanie inkubacji startowej: Pamiętaj by wykorzystywać `_caster->getCenter()` by pociski rzucały siatkę z brzucha i dłoni bohatera 3D. Odejście od tego i zastosowanie zwykłego wektora GetX/GetY wywoła wystrzeliwanie ognia z podeszwy u nóg (Y=0)!
