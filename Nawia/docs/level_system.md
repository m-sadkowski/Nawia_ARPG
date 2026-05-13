# System leveli, lokacji i encji

Ten dokument jest skrotem architektury runtime. Szczegolowy workflow dodawania
leveli jest w `docs/guides/Level_Creator_Workflow.md`.

## Architektura

```text
Engine
  LevelManager
    Level
      LocationJsonLoader
      Map
      SpawnManager
        EntityFactory
          EntityManager
```

Glowne dane:

- `assets/maps/` - modele map i pliki lighting JSON,
- `assets/data/locations/` - pliki lokacji oraz odpowiadajace im `objects_*.json`,
- `assets/data/items.json` - baza itemow,
- `assets/data/loottables.json` - tabele lootu,
- `assets/data/abilities.json` - statystyki umiejetnosci,
- `assets/data/quests.json` - konfiguracja questow,
- `assets/data/bosses.json` - definicje bossow bez pozycji spawnu.

## Wejscie na level

Typowy przeplyw:

1. `LevelManager::changeLevel(...)` wybiera level.
2. `Level::onEnter(...)` wywoluje `loadLocations(...)`.
3. `LocationJsonLoader` czyta pliki lokacji i `objects_*.json`.
4. `Map` laduje model aktualnej lokacji i buduje navmesh.
5. `SpawnManager` tworzy encje ze wszystkich lokacji.
6. Encje aktualnej lokacji sa aktywne, pozostale sa dormant.
7. Gracz trafia na `player_spawn` lokacji startowej.

Modele map ze wszystkich lokacji sa preloadowane przy starcie levelu, zeby
teleporty i respawn nie musialy tworzyc zasobow w najgorszym momencie.

## Lokacje i dormant

Encja jest aktywna, gdy:

- nalezy do aktualnej lokacji,
- ma `trigger_radius == 0` albo zostala juz aktywowana proximity.

Encja jest dormant, gdy:

- nalezy do innej lokacji,
- czeka na podejscie gracza,
- zostala zamrozona przez zmiane lokacji.

Dormant encja nie renderuje sie, nie aktualizuje AI, nie bierze udzialu w
kolizjach i nie reaguje na hover.

## Powiazane dokumenty

- `docs/guides/Level_Creator_Workflow.md`
- `docs/guides/Level_Guide.md`
- `docs/guides/Entity_Guide.md`
- `docs/guides/Interactive_Guide.md`
- `docs/guides/Item_Guide.md`
- `docs/guides/UI_Guide.md`
