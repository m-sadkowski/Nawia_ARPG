# Entity Identity and Damage Context

## Cel

Kazda encja w aktywnym swiecie gry dostaje stabilne `EntityId` na czas sesji.
ID rozroznia konkretne instancje tego samego typu, np. trzy pajaki stojace
obok siebie. Systemy ET powinny uzywac `entity_id`, a nie nazwy, typu ani
adresu obiektu w pamieci.

## Glowny kod

- `src/entity/Entity.h`
- `src/entity/Entity.cpp`
- `src/core/game/EntityManager.cpp`
- `src/core/game/combat/CombatEventBus.cpp`
- `src/core/game/agent/AgentPerceptionSystem.cpp`

## Najwazniejsze elementy

### `Entity::getEntityId()`

Zwraca ID nadane przez `EntityManager`. Wartosc `0` oznacza brak ID i nie
powinna pojawiac sie w aktywnych encjach po dodaniu ich do swiata.

### `EntityManager::assignEntityIdIfMissing(...)`

Nadaje ID przy `addEntity(...)` oraz `setPlayer(...)`. ID nie zmienia sie
podczas zycia encji.

### `DamageSourceContext`

Przechowuje zrodlo obrazen: `source_id`, nazwe, typ, frakcje, pozycje,
weak pointer do encji oraz etykiete ataku/statusu. Dzieki temu delayed damage,
np. trucizna, moze wskazywac konkretnego pajaka i konkretna etykiete
`Spider Poison`, nawet jesli cel w miedzyczasie zostanie trafiony przez inna
encje.

### `takeDamage(damage, source_context)`

Ustawia kontekst obrazen i wykonuje normalna sciezke `takeDamage`. Istniejace
wywolania `rememberDamageSource(...)` nadal dzialaja, ale nowe efekty
opoznione powinny preferowac jawny `DamageSourceContext`.

## Telemetria

JSON telemetryki zawiera `entity_id`. Pole `runtime_id` jest chwilowo wysylane
jako alias tej samej wartosci dla kompatybilnosci z istniejacym monitorem i
starymi logami. Nowe systemy powinny czytac `entity_id`.
