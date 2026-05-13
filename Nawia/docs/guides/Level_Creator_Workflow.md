# Workflow tworzenia nowego poziomu kreatorem

Ten przewodnik opisuje najprostszy dzialajacy przeplyw: dodajesz klase levelu,
wskazujesz w niej nazwy lokacji i pliki JSON, a same mapy, spawny, teleporty,
skrzynie, NPC, propsy, checkpointy i boss triggery ukladasz w kreatorze.

## 1. Ustal poziom i lokacje

Przyklad:

- poziom w menu: `Bagna Czarownicy`,
- lokacje: `Mokradla`, `Chata Czarownicy`, `Podziemia`,
- pliki lokacji: `mokradla.json`, `chata_czarownicy.json`, `podziemia.json`.

Nazwy lokacji sa wazne. Teleporty porownuja zwykle stringi, wiec
`target_location` musi wskazywac dokladnie taka nazwe, jaka podasz w levelu.

## 2. Dodaj klase levelu

Header, np. `src/world/level/BagnaCzarownicyLevel.h`:

```cpp
#pragma once

#include <Level.h>

#include <string>
#include <vector>

namespace Nawia::World {

	class BagnaCzarownicyLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override {
			return "Bagna Czarownicy";
		}

		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {
				"Mokradla",
				"Chata Czarownicy",
				"Podziemia",
			};
		}

	};

} // namespace Nawia::World
```

Implementacja, np. `src/world/level/BagnaCzarownicyLevel.cpp`:

```cpp
#include "BagnaCzarownicyLevel.h"

#include <Engine.h>
#include <Logger.h>

namespace Nawia::World {

	namespace {
		const std::vector<LevelLocationFile> BAGNA_LOCATIONS = {
			{"Mokradla", "assets/data/locations/mokradla.json"},
			{"Chata Czarownicy", "assets/data/locations/chata_czarownicy.json"},
			{"Podziemia", "assets/data/locations/podziemia.json"},
		};
	}

	void BagnaCzarownicyLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu Bagna Czarownicy...");

		loadLocations(engine, BAGNA_LOCATIONS, "Mokradla");
	}

} // namespace Nawia::World
```

Potem rejestrujesz poziom w `Engine.cpp`:

```cpp
#include <BagnaCzarownicyLevel.h>

// ...

_level_manager->registerLevel(std::make_shared<World::BagnaCzarownicyLevel>());
```

Po dodaniu nowych plikow C++ trzeba przebudowac projekt. Jezeli klasa juz
istnieje, a zmieniasz tylko JSON-y, zwykle wystarczy ponownie uruchomic gre.

## 3. Stworz lokacje w kreatorze

W menu wybierz `Kreator leveli`.

Dla kazdej lokacji:

1. Wybierz `Nowa lokacja`.
2. Wpisz nazwe identyczna jak w kodzie, np. `Mokradla`.
3. Wpisz plik, np. `mokradla.json`.
4. Wybierz model mapy albo zostaw placeholder.
5. Ustaw skale, offset, obrot i minimalna wysokosc NavMesh.
6. Kliknij `Przeladuj`, zeby zobaczyc model z ustawieniami.
7. Ustaw gracza w miejscu startowym i kliknij `Ustaw spawn`.
8. Dodaj obiekty z prawej kolumny.
9. Kliknij `Zapisz`.

Kreator zapisze dwa pliki:

```text
assets/data/locations/mokradla.json
assets/data/locations/objects_mokradla.json
```

Przy nadpisywaniu kreator poprosi o potwierdzenie.

## 4. Gdzie sa zapisywane pliki

Kreator zapisuje pliki do katalogu `assets/data/locations` wzgledem aktualnego
katalogu uruchomienia gry.

Najczestsze przypadki:

- jesli gra jest uruchomiona z katalogu builda, pliki trafia tutaj:

```text
Nawia/out/build/x64-Release/assets/data/locations/
```

- zeby pliki byly widoczne w repozytorium i mozna je bylo commitowac, musza
  trafic tutaj:

```text
Nawia/assets/data/locations/
```

Czyli po zapisaniu lokacji w kreatorze sprawdz, gdzie faktycznie powstaly pliki.
Jesli sa w katalogu builda, przenies albo skopiuj oba pliki do zrodlowych
assetow:

```text
Nawia/assets/data/locations/mokradla.json
Nawia/assets/data/locations/objects_mokradla.json
```

Po przeniesieniu do `Nawia/assets` nastepny build skopiuje je z repo do katalogu
uruchomieniowego obok `Nawia.exe`. Jesli chcesz zobaczyc zmiane bez przebudowy,
skopiuj te same pliki takze do aktualnego katalogu uruchomieniowego gry, czyli
zwykle do `Nawia/out/build/x64-Release/assets/data/locations/`.

W kodzie levelu wskazujesz zawsze sciezke assetowa:

```cpp
{"Mokradla", "assets/data/locations/mokradla.json"}
```

## 5. Teleporty

Teleport dodajesz w miejscu, w ktorym stoi gracz. W formularzu teleportu
wybierasz lokacje docelowa z listy istniejacych lokacji.

Przyklad ukladu:

- w `Mokradla` stawiasz teleport do `Chata Czarownicy`,
- w `Chata Czarownicy` stawiasz teleport powrotny do `Mokradla`,
- w `Chata Czarownicy` stawiasz teleport do `Podziemia`.

Po wejsciu w teleport runtime przeladuje model mapy i navmesh, przeniesie
gracza na spawn lokacji docelowej oraz aktywuje encje tej lokacji z puli
zaladowanej przy starcie levelu.

## 6. Format plikow

Plik lokacji:

```json
{
    "name": "Mokradla",
    "map": {
        "model": "forest.glb",
        "scale": 2.0,
        "offset": { "x": 0.0, "y": 0.0, "z": 0.0 },
        "rotation": { "x": 0.0, "y": 0.0, "z": 0.0 }
    },
    "navmesh": {
        "min_walkable_height": -14.5
    },
    "player_spawn": { "x": -4.3, "y": 33.0 },
    "objects_file": "objects_mokradla.json"
}
```

Plik obiektow:

```json
{
    "location": "Mokradla",
    "entities": [
        {
            "type": "teleport",
            "name": "Przejscie do Chaty",
            "target_location": "Chata Czarownicy",
            "x": 10.0,
            "y": 4.0
        }
    ]
}
```

Loader dopisuje lokacje obiektom automatycznie na podstawie aktualnie ladowanej
lokacji, wiec w `objects_*.json` nie trzeba recznie uzupelniac pola
`location`.

## 7. Co jest teraz obslugiwane

Runtime loader czyta:

- model mapy, skale, offset i obrot,
- `navmesh.min_walkable_height`,
- `player_spawn`,
- plik `objects_*.json`,
- encje tworzone przez `EntityFactory`, w tym teleporty i boss triggery.

Modele map oraz encje ze wszystkich lokacji levelu sa ladowane przy starcie.
Przy zmianie lokacji przeladowywana jest geometria mapy, a encje sa tylko
usypiane albo aktywowane.

## 8. Szybki checklist

Nowy poziom dziala, gdy:

- klasa dziedziczy po `Level`,
- `getLocations()` zwraca nazwy wszystkich lokacji,
- `loadLocations(engine, ..., "Lokacja startowa")` wskazuje pliki JSON,
- pliki istnieja w zrodlowym `Nawia/assets/data/locations`,
- jesli gra jest odpalana z builda, pliki sa tez skopiowane do buildowego `assets/data/locations`,
- teleporty maja poprawne `target_location`,
- level jest zarejestrowany w `Engine.cpp`,
- gra zostala przebudowana po dodaniu klasy albo uruchomiona ponownie po zmianie JSON-ow.
