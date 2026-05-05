# System leveli, lokacji i encji

Ten dokument jest skrotem architektury runtime. Szczegolowy workflow dodawania leveli jest w `docs/guides/Level_Guide.md`.

## Architektura

```text
Engine
  LevelManager
    Level
      Map
      SpawnManager
        EntityFactory
          EntityManager
```

Glowne dane:

- `assets/maps/` - modele map i pliki lighting JSON,
- `assets/data/level_entities/` - spawny per level,
- `assets/data/items.json` - baza itemow,
- `assets/data/loottables.json` - tabele lootu,
- `assets/data/abilities.json` - statystyki umiejetnosci,
- `assets/data/quests.json` - konfiguracja questow.

## Wejscie na level

Typowy przeplyw:

1. `LevelManager::loadLevel(...)` wybiera level.
2. `Level::onEnter(...)` tworzy `Map` i laduje model mapy.
3. `Map::loadMap(...)` buduje navmesh z geometrii modelu.
4. `Level::loadSpawns(...)` odpala `SpawnManager`.
5. `SpawnManager` wczytuje JSON i tworzy encje przez `EntityFactory`.
6. Gracz trafia na spawn aktualnej lokacji.

Encje ze wszystkich lokacji moga zostac utworzone od razu, ale nie wszystkie sa aktywne.

## Lokacje i dormant

Encja jest aktywna, gdy:

- nalezy do aktualnej lokacji,
- ma `trigger_radius == 0` albo zostala juz aktywowana proximity.

Encja jest dormant, gdy:

- nalezy do innej lokacji,
- czeka na podejscie gracza,
- zostala zamrozona przez zmiane lokacji.

Dormant encja nie renderuje sie, nie aktualizuje AI, nie bierze udzialu w kolizjach i nie reaguje na hover.

## Plik `level_entities`

Minimalny przyklad:

```json
{
    "player_spawn": {
        "Demo Arena": { "x": 8.0, "y": 24.0 }
    },
    "entities": [
        {
            "location": "Demo Arena",
            "type": "bandit",
            "name": "Bandyta",
            "x": 15.0,
            "y": 12.0,
            "hp": 80,
            "trigger_radius": 12.0,
            "abilities": ["KnifeThrow"]
        }
    ]
}
```

Wspolne pola:

- `location` - nazwa lokacji,
- `type` - typ obslugiwany przez `EntityFactory`,
- `name` - nazwa encji,
- `x`, `y` - pozycja logiczna,
- `hp` - zdrowie, jesli typ go uzywa,
- `trigger_radius` - aktywacja proximity,
- `spawn_radius` - losowe rozrzucenie pozycji,
- `respawnable`, `respawn_cooldown` - dane pod respawn.

## Typy encji

Aktualnie wykorzystywane typy:

- enemy: `bandit`, `devil`, `walking_dead`,
- ally: `friend`,
- interakcje: `chest`, `npc`, `teleport`, `checkpoint`,
- dekoracje: `static_object`.

Nowy typ powinien przejsc przez `EntityFactory`, nie przez hardcoded tworzenie w levelu.

## Questy, dialogi i NPC

Questy sa w duzej mierze data-driven przez `quests.json` i `QuestManager`.

Dialog kota i czesc nagrod NPC nadal maja logike C++. To jest swiadomy stan przejsciowy. Docelowo:

- dialogi powinny trafic do danych,
- NPC powinien dostawac `dialogue_id` i `quest_id` z JSON,
- nagrody powinny byc realizowane przez quest system, a nie bezposrednio w klasie NPC.

## Powiazane dokumenty

- `docs/guides/Level_Guide.md`
- `docs/guides/Entity_Guide.md`
- `docs/guides/Interactive_Guide.md`
- `docs/guides/Item_Guide.md`
- `docs/guides/UI_Guide.md`
