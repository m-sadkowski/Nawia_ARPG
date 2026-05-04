# Przewodnik po levelach, mapach i navmeshach

Ten dokument opisuje aktualny workflow dodawania nowego levela w `Nawia`:
- mapa 3D trafia do `assets/maps/`
- level laduje ja przez `Core::Map`
- encje sa definiowane w `assets/data/level_entities/*.json`
- navmesh buduje sie automatycznie z geometrii wczytanego modelu

## Najwazniejsza zasada

Mapa i navmesh nie sa dzis utrzymywane jako dwa osobne pliki. Po wywolaniu:

```cpp
_map->loadMap("forest.glb", 2.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
```

`Map`:
- laduje model z `assets/maps/forest.glb`
- ustawia transformacje modelu
- przebudowuje navmesh przez `NavMesh::buildFromModel(...)`

To oznacza, ze jakosc nawigacji zalezy bezposrednio od geometrii w pliku `.glb`.

## Jak dodac nowy level

### 1. Dodaj mape do assetow

Wrzuc plik `.glb` albo `.gltf` do `assets/maps/`.

Przyklad:

```text
assets/maps/moja_dolina.glb
```

### 2. Dodaj klase levela

Utworz plik naglowkowy, na przyklad `src/world/level/MojaDolinaLevel.h`:

```cpp
#pragma once

#include "Level.h"

namespace Nawia::World {

class MojaDolinaLevel : public Level {
public:
    void onEnter(Core::Engine* engine) override;

    [[nodiscard]] std::string getName() const override { return "Moja Dolina"; }

    [[nodiscard]] std::string getSpawnFilePath() const override {
        return "../assets/data/level_entities/moja_dolina.json";
    }

    [[nodiscard]] std::vector<std::string> getLocations() const override {
        return { "Polana", "Jaskinia" };
    }
};

} // namespace Nawia::World
```

Implementacja:

```cpp
#include "MojaDolinaLevel.h"

#include <Engine.h>
#include <Logger.h>
#include <Map.h>

namespace Nawia::World {

void MojaDolinaLevel::onEnter(Core::Engine* engine) {
    Core::Logger::debugLog("Ladowanie poziomu Moja Dolina...");

    _map = std::make_unique<Core::Map>(engine->getResourceManager());
    _map->loadMap("moja_dolina.glb", 1.5f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });

    engine->getEntityManager().clearNonPlayerEntities();
    loadSpawns(engine);
}

} // namespace Nawia::World
```

### 3. Zarejestruj level w `Engine`

W konstruktorze `Engine` dodaj:

```cpp
_level_manager->registerLevel(std::make_shared<World::MojaDolinaLevel>());
```

### 4. Dodaj plik encji

Utworz `assets/data/level_entities/moja_dolina.json`.

Przyklad:

```json
{
    "player_spawn": {
        "Polana": { "x": 0.0, "y": 0.0 },
        "Jaskinia": { "x": 12.0, "y": -8.0 }
    },
    "entities": [
        {
            "location": "Polana",
            "type": "devil",
            "name": "Straznik",
            "x": 8.0,
            "y": 4.0,
            "hp": 150,
            "trigger_radius": 12.0
        }
    ]
}
```

## Jak dodac nowa mape poprawnie

Z punktu widzenia kodu level potrzebuje tylko poprawnego wywolania `loadMap(...)`, ale w praktyce warto przejsc ta checkliste:

1. Umiesc plik mapy w `assets/maps/`.
2. Ustal docelowe `scale`, `offset` i `rotation` w kodzie levela.
3. Uruchom poziom i sprawdz:
   - czy raycast myszy trafia w teren
   - czy gracz chodzi tam, gdzie powinien
   - czy pathfinding nie tnie przez sciany
4. Jesli poziom ma swoje swiatlo, dodaj tez plik `assets/maps/<nazwa>_lighting.json`.

## Jak dzis dziala navmesh

Aktualny system:
- bierze wszystkie meshe z modelu
- transformuje je przez `model.transform`
- stosuje `scale`
- buduje navmesh w runtime przez Recast/Detour

To daje prosty pipeline, ale ma tez konsekwencje:

1. Jesli w modelu sa duze dekoracje albo sufity, tez wejda do budowy navmesha.
2. Jesli teren jest bardzo gesty albo brudny topologicznie, navmesh moze byc slabej jakosci.
3. `offset` nie zmienia samej siatki navmesha, ale `rotation` i `scale` juz tak, bo sa uwzgledniane przy budowie z `model.transform`.

## Rekomendacje dla authoringu map

Zeby obecny auto-navmesh dzialal dobrze:

1. Trzymaj teren mozliwie czysty i prosty.
2. Unikaj wrzucania do glownego modelu niepotrzebnych, ogromnych meshy kolizyjnych.
3. Jesli dekoracje mocno psuja nawigacje, rozdziel mape na bardziej przewidywalna geometrie.
4. Po kazdej zmianie geometrii przetestuj:
   - ruch gracza
   - klik-move
   - przeciwnikow korzystajacych z pathfindingu

## Kiedy potrzebny jest osobny plik navmesha

Na ten moment nie jest potrzebny, bo silnik go nie wczytuje. Jedynym zrodlem danych dla navmesha jest wczytany model mapy.

Jesli w przyszlosci dojdzie osobny mesh tylko pod nawigacje, dokumentacja powinna zostac rozszerzona o osobny pipeline eksportu.

## Typowe problemy

### Gracz moze chodzic tam, gdzie nie powinien

Najczestsze przyczyny:
- zbyt uproszczona geometria mapy
- dekoracje dolaczone do glownego modelu i potraktowane jako powierzchnia chodzona
- zly `scale`

### Pathfinding nie dochodzi do kliknietego miejsca

Najczestsze przyczyny:
- klik trafia w miejsce poza navmeshem
- koniec sciezki wypada za daleko od punktu docelowego
- model ma dziury albo bardzo nieczytelne uskoki

### Po przebudowie nie widac nowych assetow

Assety z `Nawia/assets` sa kopiowane do katalogu builda po kompilacji. Jesli dodajesz nowy plik mapy albo plik lighting JSON, przebuduj projekt, zeby kopia w katalogu build byla aktualna.

## Powiazane pliki

- `src/world/level/Level.h`
- `src/world/level/Level.cpp`
- `src/core/game/Map.h`
- `src/core/game/Map.cpp`
- `src/world/NavMesh.h`
- `src/world/NavMesh.cpp`
- `src/world/spawn/SpawnManager.cpp`
