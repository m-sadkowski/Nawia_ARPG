# Standard kodu i dokumentacji

Ten dokument opisuje zasady obowiązujące w kodzie Nawia. Traktuj go jako punkt odniesienia przy nowych funkcjach, refaktorach i poprawkach po review.

## Nazewnictwo

- Typy, klasy, struktury i enumy: `PascalCase`, np. `ActorInterface`, `EntityType`.
- Namespace'y zostają w obecnym stylu projektu: `Nawia::Entity`, `Nawia::Core`, `Nawia::World`.
- Metody publicznego API zostają w obecnym stylu projektu: `camelCase`, np. `setTarget`, `getDistanceToTarget`.
- Zmienne lokalne, parametry i pola prywatne: `snake_case`.
- Pola prywatne/protected mają prefiks `_`, np. `_movement_speed`, `_attack_cooldown_timer`.
- Stałe `static constexpr`: `UPPER_SNAKE_CASE`, np. `VISION_RANGE`, `ATTACK_COOLDOWN`.
- Nazwy mają mówić, co reprezentują. Unikaj skrótów typu `tmp`, `obj`, `val`, jeśli zakres użycia nie jest natychmiast oczywisty.

## Include'y

Kolejność include'ów:

1. W pliku `.cpp` najpierw własny nagłówek implementowanej klasy w cudzysłowie, np. `#include "Bandit.h"`.
2. Pozostałe nagłówki projektu w nawiasach ostrych, np. `#include <AllyInterface.h>`, `#include <Map.h>`.
3. Zewnętrzne biblioteki w nawiasach ostrych, np. `#include <raylib.h>`, `#include <raymath.h>`.
4. Biblioteka standardowa C++ w nawiasach ostrych, np. `#include <memory>`, `#include <vector>`.

W nagłówkach `.h` wszystkie include'y zapisujemy w nawiasach ostrych, bo nie istnieje tam osobny
include własnego nagłówka implementacji.

W każdej grupie sortuj alfabetycznie. Między grupami zostaw pustą linię.

Przykład:

```cpp
#include "Bandit.h"

#include <Ability.h>
#include <Collider.h>
#include <Map.h>
#include <MathUtils.h>

#include <raymath.h>

#include <memory>
```

## Forward declaration kontra include

W nagłówkach preferuj forward declaration, gdy typ jest używany tylko przez wskaźnik, referencję lub `std::shared_ptr`/`std::unique_ptr` w deklaracji API.

Użyj pełnego include'a w nagłówku, gdy:

- dziedziczysz po klasie,
- przechowujesz typ przez wartość,
- metoda inline odwołuje się do pól albo metod tego typu,
- potrzebujesz definicji enumu, stałej lub aliasu,
- szablon wymaga pełnej definicji typu w miejscu deklaracji.

Pełne definicje przenoś do `.cpp`, jeśli są potrzebne tylko w implementacji. To zmniejsza sprzężenie i skraca przebudowy.

## Doxygen

Publiczne klasy, enumy, funkcje i nietrywialne metody protected powinny mieć krótki komentarz Doxygen po polsku.

Preferowany format:

```cpp
/**
 * @brief Aktualizuje stan AI i animacji bandyty.
 * @param dt Czas od poprzedniej klatki w sekundach.
 */
void update(float dt) override;
```

Komentarz ma odpowiadać na pytania: co robi funkcja, po co istnieje i kiedy trzeba uważać na szczegóły implementacji. Jeśli nazwa funkcji jest w pełni samowyjaśniająca, wystarczy krótki `@brief`.

Nie opisuj mechanicznie każdej linijki. Doxygen ma pomagać czytelnikowi zrozumieć kontrakt, nie przepisywać implementację.

## Formatowanie

- Projekt używa tabów do wcięć w C++.
- Maksymalna długość linii: 120 znaków, chyba że czytelność realnie cierpi na łamaniu.
- Bloki `if`, `for`, `while`, `switch` używają klamer przy logice dłuższej niż jedna bardzo prosta instrukcja.
- Unikaj wielkich funkcji. Gdy metoda zaczyna mieszać kilka decyzji, wyciągnij prywatne helpery typu `handleChasingState`, `updateCooldowns`, `tryCastAbility`.
- Nie formatuj całego projektu przy małej zmianie. Wielkie przebiegi formatterem robimy jako osobny commit/refaktor.

## Podział odpowiedzialności

- `Entity` przechowuje wspólne cechy obiektów świata: pozycję, model, animacje, HP, frakcję, target i abilities.
- `ActorInterface` zbiera wspólne zachowanie jednostek bojowych: mapa poziomu i target.
- `EnemyInterface` ustawia specjalizację wroga.
- `AllyInterface` ustawia specjalizację sojusznika i opcjonalny `AllyBrain`.
- `EntityFactory` składa obiekty z danych JSON, ale nie prowadzi AI.
- Logika decyzji powinna żyć w klasie aktora albo brainie, a nie w factory.

## Plan refaktorów

Najbezpieczniejszy podział dalszych prac:

1. Fundamenty: standard dokumentacji, include'y, forward declarations, małe pull-upy wspólnych pól.
2. Actors: wspólny model enemy/ally, redukcja duplikacji stanów, ujednolicenie `takeDamage`.
3. Abilities: wspólny kontrakt castowania, pending spawns, cooldowny i dokumentacja efektów.
4. Entity/Engine: rozbijanie długich metod, ograniczanie bezpośrednich zależności między managerami.
5. UI: wyciąganie helperów renderujących i porządek w `UIHandler`.
6. Guides: aktualizacja przewodników po każdej większej zmianie w module.
