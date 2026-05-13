# Entity module

Ten katalog zawiera glowna logike encji w projekcie.

## Najwazniejsze punkty wejscia

- `Entity.h` - baza dla wszystkich encji
- `actors/ActorInterface.h` - wspolna baza jednostek bojowych z mapa i targetem
- `actors/enemies/EnemyInterface.h` - baza dla wrogow
- `actors/allies/AllyInterface.h` - baza dla sojusznikow
- `actors/allies/AllyBrain.h` - hook pod przyszla logike ET/brain dla ally
- `abilities/` - logika skilli
- `collider/` - collidery dla efektow, triggerow i wybranych encji

## Aktualny model tworzenia actorow

Nowe jednostki bojowe zwykle:

1. dziedzicza po `EnemyInterface` albo `AllyInterface`, ktore wspolnie korzystaja z `ActorInterface`
2. maja prywatny konstruktor
3. maja builder
4. sa rejestrowane w `src/world/spawn/EntityFactory.cpp`
5. sa spawnione przez JSON w `assets/data/locations/objects_*.json`

## Aktualny model AI

- enemy dostaja target centralnie przez `EntityManager`
- ally dostaja target centralnie przez `EntityManager`
- ally moga dzialac:
  - hardcoded
  - albo przez `AllyBrain`

Przyklad aktualnego ally:

- `actors/allies/friend/Friend.h`
- `actors/allies/friend/Friend.cpp`

## Dokumentacja

Szczegoly sa w:

- `docs/guides/Coding_Standards.md`
- `docs/guides/Entity_Guide.md`
- `docs/guides/Enemy_Guide.md`
- `docs/guides/Ability_Guide.md`
- `docs/guides/AbilityEffect_Guide.md`
- `docs/guides/Interactive_Guide.md`
