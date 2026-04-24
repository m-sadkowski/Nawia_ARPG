# Przewodnik po systemie Leveli i Encji

System leveli w grze Nawia ARPG opiera się na hierarchii `Level` → `Location` → `Entity`, gdzie każdy level definiuje swoją geometrię 3D, listę lokacji oraz plik JSON z encjami. Encje (wrogowie, skrzynki, NPC, checkpointy) są **pre-tworzone przy wejściu na mapę** i aktywowane na podstawie odległości gracza.

## Architektura

```
Engine
 └── LevelManager
      └── Level (baza abstrakcyjna)
           ├── _map (Map — geometria 3D)
           ├── _spawn_manager (SpawnManager — pre-tworzy i aktywuje encje)
           └── getLocations() → lista nazw lokacji
```

Każdy level ma swój plik JSON w `assets/data/level_entities/`, który definiuje:
- Pozycję startową gracza per lokacja
- Wszystkie encje (wrogowie, skrzynki, NPC, checkpointy, obiekty statyczne)

## Jak stworzyć nowy level?

### Krok 1: Nagłówek (`src/world/level/NowyLevel.h`)

```cpp
#pragma once
#include "Level.h"

namespace Nawia::World {

class NowyLevel : public Level {
public:
    void onEnter(Core::Engine* engine) override;

    [[nodiscard]] std::string getName() const override { return "Nowy Level"; }

    [[nodiscard]] std::string getSpawnFilePath() const override {
        return "../assets/data/level_entities/nowy_level.json";
    }

    [[nodiscard]] std::vector<std::string> getLocations() const override {
        return {"Lokacja A", "Lokacja B"};
    }
};

} // namespace Nawia::World
```

### Krok 2: Implementacja (`src/world/level/NowyLevel.cpp`)

```cpp
#include "NowyLevel.h"
#include <Map.h>
#include <Engine.h>
#include <Logger.h>

namespace Nawia::World {

void NowyLevel::onEnter(Core::Engine* engine) {
    Core::Logger::debugLog("Ladowanie: Nowy Level...");

    // 1. Załaduj mapę 3D
    _map = std::make_unique<Core::Map>(engine->getResourceManager());
    _map->loadMap("../assets/maps/nowy_level.glb", 2.0f);

    // 2. Wyczyść encje z poprzedniego levelu
    engine->getEntityManager().clearNonPlayerEntities();

    // 3. Załaduj encje z JSON (pre-tworzy wszystkie, ustawia pozycję gracza)
    loadSpawns(engine);
}

} // namespace Nawia::World
```

### Krok 3: Rejestracja w Engine

W `Engine.cpp`, w konstruktorze dodaj:
```cpp
_level_manager->registerLevel(std::make_shared<World::NowyLevel>());
```

### Krok 4: Plik JSON (`assets/data/level_entities/nowy_level.json`)

```json
{
    "player_spawn": {
        "Lokacja A": { "x": 5.0, "y": 10.0 },
        "Lokacja B": { "x": 0.0, "y": 0.0 }
    },
    "entities": [
        {
            "location": "Lokacja A",
            "type": "devil",
            "name": "Demon Strażnik",
            "x": 10.0, "y": 15.0,
            "hp": 200,
            "trigger_radius": 12.0
        },
        {
            "location": "Lokacja A",
            "type": "chest",
            "name": "Skrzynia",
            "x": 3.0, "y": 8.0,
            "loottable": "CHEST_NOOB",
            "trigger_radius": 0
        }
    ]
}
```

## System Dormant — encje śpią do momentu zbliżenia gracza

Wszystkie encje są tworzone (modele, tekstury, animacje) **jednorazowo przy wejściu na mapę**. Dzięki temu nie ma lagów w trakcie rozgrywki.

### Jak to działa?

1. **`loadSpawns(engine)`** → `SpawnManager::loadFromJson()` tworzy WSZYSTKIE encje
2. Encje z `trigger_radius > 0` startują jako **dormant** (zamrożone + niewidoczne)
3. Encje z `trigger_radius == 0` są od razu aktywne (skrzynki, NPC)
4. Co klatkę `Level::update()` → `SpawnManager::update()` sprawdza odległość gracza
5. Gdy gracz wejdzie w zasięg → `entity->setDormant(false)` — encja się budzi

### Co oznacza dormant?

Encja w stanie dormant:
- ❌ Nie jest renderowana (niewidoczna)
- ❌ Nie jest aktualizowana (zamrożone AI, brak ruchu)
- ❌ Nie uczestniczy w kolizjach
- ❌ Nie reaguje na kliknięcia/hover
- ✅ Istnieje w pamięci (model załadowany, gotowa do natychmiastowej aktywacji)

### Praktyczne zastosowania

```json
{
    "type": "devil",
    "trigger_radius": 15.0
}
```
Wróg jest niewidoczny dopóki gracz nie podejdzie na 15 jednostek. Idealny do zasadzek!

```json
{
    "type": "chest",
    "trigger_radius": 0
}
```
Skrzynia jest widoczna od razu po wejściu na mapę.

## Dostępne typy encji w JSON

### Wrogowie: `devil`, `bandit`, `walking_dead`

```json
{
    "location": "Nazwa Lokacji",
    "type": "devil",
    "name": "Mocny Devil",
    "x": 10.0, "y": 15.0,
    "hp": 200,
    "trigger_radius": 15.0,
    "spawn_radius": 3.0
}
```

| Pole | Typ | Opis |
|---|---|---|
| `hp` | int | Punkty życia |
| `abilities` | string[] | (tylko bandit) np. `["KnifeThrow"]` |
| `trigger_radius` | float | Odległość aktywacji (0 = natychmiast) |
| `spawn_radius` | float | Losowy offset pozycji (wariacja dla grup) |

Wrogowie automatycznie dostają gracza jako cel (`setTarget`).

### Skrzynia: `chest`

```json
{
    "type": "chest",
    "name": "Stara Skrzynia",
    "x": -13.44, "y": -21.44,
    "loottable": "CHEST_NOOB",
    "trigger_radius": 0
}
```

| Pole | Typ | Opis |
|---|---|---|
| `loottable` | string | Tabela łupów: `CHEST_NOOB`, `CHEST_BAD`, `CAT` |
| `items` | int[] | Konkretne ID itemów |
| `locked` | bool | Czy zablokowana |
| `key_id` | int | ID klucza do odblokowania |

### NPC: `npc`

```json
{
    "type": "npc",
    "npc_class": "cat",
    "name": "Kot Olga",
    "x": 0.35, "y": -18.23,
    "loottable": "CAT",
    "trigger_radius": 0
}
```

| Pole | Typ | Opis |
|---|---|---|
| `npc_class` | string | Klasa NPC: aktualnie `"cat"` |
| `loottable` | string | Tabela łupów (opcjonalne) |

⚠️ **Dialogi i questy NPC są aktualnie hardkodowane w C++** (`DialogueManager.cpp`, `Cat.cpp`). W przyszłości zostaną przeniesione do JSONa z systemem `dialogues.json` + `quests.json`.

### Obiekt statyczny: `static_object`

```json
{
    "type": "static_object",
    "name": "Drzewo",
    "x": 5.0, "y": 5.0,
    "hp": 9999,
    "texture": "assets/textures/chest.png",
    "trigger_radius": 0
}
```

### Checkpoint: `checkpoint`

```json
{
    "type": "checkpoint",
    "name": "Punkt Kontrolny",
    "x": 20.0, "y": 20.0,
    "trigger_radius": 0
}
```

## Jak dodać nowy typ encji?

### Krok 1: Zarejestruj typ w `EntityFactory::create()`

```cpp
// W EntityFactory.cpp, metoda create():
if (type == "moj_typ") return createMojTyp(data, engine, map);
```

### Krok 2: Dodaj metodę fabryczną

```cpp
std::shared_ptr<Entity::Entity> EntityFactory::createMojTyp(
    const json& data, Core::Engine* engine, Core::Map* map)
{
    const float x = data.value("x", 0.0f);
    const float y = data.value("y", 0.0f);
    const std::string name = data.value("name", "Moj Typ");

    auto entity = std::make_shared<Entity::MojTyp>(name, x, y);
    // ... konfiguracja z JSON ...
    return entity;
}
```

### Krok 3: Użyj w JSON

```json
{
    "location": "Demo Arena",
    "type": "moj_typ",
    "name": "Przykładowy Obiekt",
    "x": 5.0, "y": 10.0,
    "trigger_radius": 0
}
```

## Wspólne pola wszystkich encji

| Pole | Typ | Domyślnie | Opis |
|---|---|---|---|
| `location` | string | (wymagane) | Nazwa lokacji z `getLocations()` |
| `type` | string | (wymagane) | Typ encji w EntityFactory |
| `name` | string | zależy od typu | Wyświetlana nazwa |
| `x`, `y` | float | 0.0 | Pozycja w świecie (XZ) |
| `trigger_radius` | float | 0.0 | 0 = aktywna od razu; >0 = dormant do zbliżenia |
| `spawn_radius` | float | 0.0 | Losowy offset pozycji |
| `respawnable` | bool | false | Czy może się odrodzić |
| `respawn_cooldown` | float | 0.0 | Czas do odrodzenia (sekundy) |

## Pliki źródłowe

| Plik | Rola |
|---|---|
| `src/entity/Entity.h/.cpp` | Flaga `_dormant`, guard w `update()` i `render()` |
| `src/Core/game/EntityManager.cpp` | Skip dormant w: render, kolizjach, hover, click |
| `src/world/level/Level.h/.cpp` | Baza abstrakcyjna, `loadSpawns()`, domyślny `update()` |
| `src/world/spawn/SpawnManager.h/.cpp` | Pre-tworzy encje, aktywuje na proximity |
| `src/world/spawn/SpawnPoint.h` | Struct spawn pointa + referencja do encji |
| `src/world/spawn/EntityFactory.h/.cpp` | Fabryka — tworzy encje z JSON po typie |
| `src/world/level/LevelManager.h/.cpp` | Rejestr leveli, przełączanie |
