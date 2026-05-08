# Przewodnik po levelach, mapach i spawnach

Ten dokument opisuje dodawanie levelu, ladowanie mapy 3D, navmesh, lokacje i encje z JSON.

## Glowny przeplyw

Level robi zwykle:

1. Tworzy `Core::Map`.
2. Laduje model mapy przez `Map::loadMap(...)`.
3. Czysci stare encje poza graczem.
4. Wczytuje spawny przez `loadSpawns(engine)`.

`Map::loadMap(...)` laduje model z `assets/maps/` i buduje navmesh z geometrii modelu.

## Dodanie nowego levelu

Header:

```cpp
#pragma once

#include <Level.h>

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

void MojaDolinaLevel::onEnter(Core::Engine* engine)
{
	Core::Logger::debugLog("Ladowanie poziomu Moja Dolina...");

	_map = std::make_unique<Core::Map>(engine->getResourceManager());
	_map->loadMap("moja_dolina.glb", 1.5f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});

	engine->getEntityManager().clearNonPlayerEntities();
	loadSpawns(engine);
}

} // namespace Nawia::World
```

Na koncu zarejestruj level w `Engine`.

## Plik `level_entities`

Kazdy level ma plik w `assets/data/level_entities/`.

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
            "type": "bandit",
            "name": "Bandyta",
            "x": 8.0,
            "y": 4.0,
            "hp": 80,
            "trigger_radius": 12.0,
            "abilities": ["KnifeThrow"]
        }
    ]
}
```

Wspolne pola:

- `location` - nazwa lokacji z `getLocations()`,
- `type` - typ obslugiwany przez `EntityFactory`,
- `name` - nazwa encji,
- `x`, `y` - pozycja logiczna X/Z,
- `hp` - zdrowie, jezeli typ tego uzywa,
- `trigger_radius` - 0 oznacza aktywna od razu, >0 aktywuje po zblizeniu,
- `spawn_radius` - opcjonalny losowy offset,
- `respawnable` i `respawn_cooldown` - przygotowane pod respawn.

## Typy encji

Aktualnie w JSON sa obslugiwane m.in.:

- `bandit`,
- `devil`,
- `walking_dead`,
- `friend`,
- `chest`,
- `npc` z `npc_class`,
- `static_object`,
- `teleport`,
- `checkpoint`.

Nowy typ dodaj w `EntityFactory`, a nie bezposrednio w levelu.

## Lokacje i dormant

Wszystkie encje levelu sa tworzone przy wejsciu na level, ale moga startowac jako dormant.

Encja jest dormant, gdy:

- nalezy do innej lokacji niz aktualna,
- ma `trigger_radius > 0` i gracz jeszcze nie podszedl.

Dormant encja nie renderuje sie, nie aktualizuje AI i nie bierze udzialu w kolizjach.

## Navmesh

Navmesh jest budowany runtime z geometrii modelu mapy.

Konsekwencje:

- dekoracje w modelu moga trafic do navmesha,
- brudna topologia pogarsza pathfinding,
- `scale` i `rotation` maja znaczenie dla budowy,
- po zmianie assetow trzeba przebudowac projekt, zeby build dostal aktualne pliki.

## Static props

Static objecty powinny byc ustawione na poziomie podlogi przez logike spawn/map, a nie przez legacy plaska plaszczyzne. Jezeli prop wisi, sprawdz:

- pozycje JSON,
- snap do mapy,
- bounding box modelu,
- skale i offset.

## Lighting

Poziom moze ladowac konfiguracje swiatla z JSON w `assets/maps/..._lighting.json`.

## Kreator Poziomów (DevLevel)

Zalecanym sposobem tworzenia danych dla nowych poziomów jest użycie wbudowanego **Kreatora Poziomów (`DevLevel`)**. 
Pozwala on na wizualne rozstawianie obiektów, spawnerów i skrzyń bezpośrednio w świecie gry, co jest znacznie szybsze i bezpieczniejsze niż ręczna edycja plików JSON.

Więcej szczegółów znajdziesz w: [Przewodnik po DevLevel](DevLevel_Guide.md).

## Dobre praktyki

- Level nie powinien recznie tworzyc zwyklych enemy/NPC.
- Nowy gameplay spawnuj przez JSON i `EntityFactory`.
- Po zmianie mapy testuj klik-move, enemy pathfinding i teleporty.
- Nie mieszaj danych kilku leveli w jednym pliku spawn.
- Zawsze zakladaj uruchamianie z `out/build/x64-Release`.
