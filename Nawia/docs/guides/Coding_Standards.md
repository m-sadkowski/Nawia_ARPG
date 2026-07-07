# Standard kodu i dokumentacji

Ten dokument jest zrodlem prawdy dla stylu kodu w Nawia. Stosuj go przy nowych funkcjach, refaktorach i poprawkach po review.

## Nazewnictwo

- Klasy, struktury, enumy i aliasy typow: `PascalCase`, np. `EntityFactory`, `EquipmentSlot`.
- Namespace'y zostaja w stylu projektu: `Nawia::Entity`, `Nawia::Core`, `Nawia::World`, `Nawia::UI`.
- Metody publiczne: `camelCase`, np. `setTarget`, `getDistanceToTarget`, `loadResources`.
- Zmienne lokalne, parametry i pola pomocnicze: `snake_case`.
- Pola prywatne/protected maja prefiks `_`, np. `_movement_speed`, `_attack_cooldown_timer`.
- Stale `static constexpr`: `UPPER_SNAKE_CASE`, np. `ATTACK_RANGE`, `DIALOGUE_BOX_MARGIN`.
- Nazwy maja opisywac role obiektu. Unikaj `tmp`, `obj`, `val`, jezeli zakres nie jest oczywisty.

## Include'y

Docelowy format:

1. W `.cpp` pierwszy jest wlasny header klasy w cudzyslowie.
2. Wszystkie pozostale headery projektu sa w nawiasach ostrych.
3. Biblioteki zewnetrzne sa w nawiasach ostrych.
4. Biblioteka standardowa C++ jest w nawiasach ostrych.

W `.h` wszystkie include'y sa w nawiasach ostrych. Header nie ma "wlasnego" include'a.

Miedzy grupami zostaw pusta linie. W grupach sortuj alfabetycznie, o ile nie pogarsza to czytelnosci.

Przyklad:

```cpp
#include "Bandit.h"

#include <Ability.h>
#include <Collider.h>
#include <Map.h>

#include <raymath.h>

#include <memory>
#include <vector>
```

## Forward declaration kontra include

W headerach preferuj forward declaration, gdy typ wystepuje tylko jako wskaznik, referencja albo smart pointer w deklaracji API.

Pelny include w headerze jest potrzebny, gdy:

- dziedziczysz po typie,
- trzymasz typ przez wartosc,
- uzywasz metod albo pol typu w metodzie inline,
- potrzebujesz definicji enumu, stalej lub aliasu,
- szablon wymaga pelnej definicji w miejscu deklaracji.

Pelne zaleznosci przenos do `.cpp`, jesli sa potrzebne tylko w implementacji. To zmniejsza sprzezenie modulow i skroci przebudowy.

## Dokumentacja Doxygen

Publiczne klasy, enumy, funkcje i nietrywialne metody protected powinny miec krotki komentarz Doxygen po polsku.

Preferowany format:

```cpp
/**
 * @brief Aktualizuje stan zachowania i animacji bandyty.
 * @param dt Czas od poprzedniej klatki w sekundach.
 */
void update(float dt) override;
```

Komentarz ma opisac kontrakt: co metoda robi, po co istnieje i kiedy trzeba uwazac. Nie opisuj mechanicznie kazdej linijki. Jezeli nazwa jest oczywista, wystarczy krotki `@brief`.

Komentarze w kodzie pisz po polsku. W nowych plikach trzymaj ASCII, zeby unikac problemow kodowania w Visual Studio i terminalu.

## Formatowanie

- Nie formatuj calego projektu przy malej zmianie.
- Rozbijaj dlugie funkcje na prywatne helpery, gdy metoda miesza kilka decyzji.
- Preferuj wczesne wyjscia dla guardow, ale nie kosztem czytelnosci.
- Klamry dodawaj przy nietrywialnych blokach. Jedna bardzo prosta instrukcja moze zostac w jednej linii tylko wtedy, gdy jest czytelna.
- Nie dodawaj abstrakcji "na zapas". Interfejs ma usuwac realna duplikacje albo ograniczac sprzezenie.

## Wlasnosc i pointery

- `std::unique_ptr` oznacza jednoznaczne posiadanie, np. komponent UI posiadany przez `UIHandler`.
- `std::shared_ptr` stosuj dla obiektow wspoldzielonych w runtime, np. encji w `EntityManager`, template'ow itemow, tekstur z cache.
- `std::weak_ptr` stosuj dla relacji, ktore nie powinny przedluzac zycia obiektu, np. target encji.
- Surowy wskaznik jest dopuszczalny jako nieposiadajaca referencja do systemu zyjacego dluzej, np. manager w `Engine`.
- Nie przekazuj ownership przez surowe wskazniki.

## Podzial odpowiedzialnosci

- `Entity` zbiera wspolny stan obiektow swiata: pozycje, model, animacje, HP, frakcje, target, abilities i pending spawns.
- `ActorInterface` trzyma wspolne elementy jednostek bojowych: mape i target.
- `EnemyInterface` i `AllyInterface` dodaja specjalizacje bojowa.
- `EntityFactory` sklada encje z JSON, ale nie prowadzi zachowania aktorow.
- `SpawnManager` zarzadza aktywacja encji per lokacja i proximity.
- `UIHandler` koordynuje ekrany UI i HUD, a mniejsze klasy rysuja konkretne panele.

## Checklist review

Przed oddaniem zmiany sprawdz:

1. Czy include'y sa w docelowym formacie.
2. Czy nowe publiczne API ma Doxygen po polsku.
3. Czy komentarze nie sa po angielsku.
4. Czy zaleznosci w headerach da sie zastapic forward declaration.
5. Czy pointery pokazuja realna wlasnosc obiektu.
6. Czy dluga metoda nie prosi sie o helper.
7. Czy zmiana nie dodaje logiki gameplayowej do factory albo UI.
