# Combat Event Hooks

## Cel

`CombatEventBus` jest niskopoziomowym strumieniem faktow z walki. Nie zawiera
logiki AI, threat table, walidacji ani rol. Jego zadaniem jest zapisanie tego,
co realnie wydarzylo sie w grze, tak aby pozniejsze systemy pracy
inzynierskiej mogly budowac na tych danych percepcje, metryki, aggro,
adaptacje i debugowanie zachowan agentow.

## Glowny kod

- `src/core/game/combat/CombatEventBus.h`
- `src/core/game/combat/CombatEventBus.cpp`
- `src/entity/Entity.h`
- `src/entity/Entity.cpp`
- `docs/ET systems/Entity_Identity_and_Damage_Context.md`
- `src/entity/abilities/Ability.cpp`

`Engine` posiada instancje `Game::CombatEventBus` i aktualizuje jej czas w
kazdej klatce. `Entity` dostaje wskaznik do busa przez statyczny accessor,
podobnie jak obecny shared `ResourceManager`.

## Typy eventow

### `DamageDealt`

Emitowany centralnie w `Entity::takeDamage`.

Zawiera:

- `source` - kto zadal obrazenia, jesli znany,
- `target` - kto otrzymal obrazenia,
- `entity_id` w `source` i `target` - stabilne ID konkretnej instancji encji,
- `source_label` - nazwa ability albo ataku, np. `Fireball`, `Devil Dash`,
- `amount` - zadane obrazenia po modyfikatorach,
- `hp_before` i `hp_after`,
- `lethal` - czy trafienie sprowadzilo HP do zera.

### `EntityKilled`

Emitowany, gdy encja rozpoczyna sekwencje smierci po lethal damage albo gdy
zostaje zabita przez bezposrednie `Entity::die`.

Event ma zabezpieczenie przed duplikatami przez flage
`_combat_death_event_emitted`.

### `AbilityCastStarted`

Emitowany w `Ability::beginCast`.

Zawiera:

- `source` - caster,
- `source_label` - nazwa ability,
- `target_position`, jesli ability przekazalo punkt celu.

## Najwazniejsze funkcje interfejsu

### `CombatEventBus::update(float dt)`

Przesuwa wewnetrzny zegar eventow. Wolane przez `Engine::update`.

### `CombatEventBus::emitDamageDealt(...)`

Dodaje event obrazen. Normalnie nie powinno byc wolane recznie z klas
konkretnych przeciwnikow, bo centralnym miejscem emisji jest
`Entity::takeDamage`.

### `CombatEventBus::emitEntityKilled(...)`

Dodaje event smierci. Wolane przez `Entity::takeDamage` i `Entity::die`.

### `CombatEventBus::emitAbilityCastStarted(...)`

Dodaje event rozpoczecia uzycia ability. Wolane przez `Ability::beginCast`.

### `CombatEventBus::getRecentEvents(float seconds)`

Zwraca kopie eventow z ostatnich `seconds` sekund. To bedzie podstawowy punkt
wejscia dla przyszlego `AgentPerception`, np. "co stalo sie w ostatnich 2 s".

### `CombatEventBus::subscribe(Listener listener)`

Pozwala systemowi reagowac natychmiast na nowe eventy. Zwraca
`SubscriptionId`, ktory mozna potem przekazac do `unsubscribe`.

## Zasada atrybucji obrazen

Przed wywolaniem `target->takeDamage(...)` kod powinien ustawic zrodlo:

```cpp
target->rememberDamageSource(caster, "Fireball");
target->takeDamage(damage);
```

`Entity::takeDamage` odczytuje ostatnie zrodlo, emituje event i czysci tylko
etykiete `source_label`. Sam weak pointer do agresora zostaje, bo obecny
`EntityManager` uzywa go do wyboru celu po otrzymaniu obrazen.

Dla obrazen opoznionych, np. trucizny, nalezy przekazac jawny
`DamageSourceContext`. Wtedy kazdy tick statusu nadal wskazuje konkretna
instancje zrodla, np. `Spider#42`, zamiast przypadkowo uzyc ostatniego
agresora trafionego celu.

## Istniejacy kod kluczowy dla ET

### `Entity`

Najwazniejszy punkt integracji. Zawiera HP, frakcje, typ encji, target,
ability, ruch, smierc i teraz emisje eventow walki.

### `Ability`

Wspolna baza ability. `beginCast` jest miejscem, w ktorym rejestrowane jest
rozpoczecie castu. Dzieki temu przyszle cast windows, interrupt windows i
percepcja agentow moga korzystac z jednego zrodla danych.

### `EntityManager`

Nadal odpowiada za update encji, kolizje ability i targetowanie. W przyszlosci
nie powinien implementowac AI ani threat table; powinien raczej dostarczac
aktywny swiat, z ktorego korzysta percepcja.

### `BossManager`

Juz ma fazy, miniony i stan aktywnej walki. Dalsze raid-like mechaniki powinny
emitowac eventy przez te same hooki, zamiast tworzyc osobny system logowania.

## Granice systemu

Ten system nie decyduje:

- kto ma miec aggro,
- czy agent powinien uniknac AoE,
- jak mierzyc skutecznosc AI,
- jak koordynowac wielu agentow,
- czy dana akcja byla dobra.

To wszystko nalezy do pozniejszych systemow inzynierskich. `CombatEventBus`
ma tylko dostarczyc wiarygodne, spojne dane wejsciowe.

## Podglad w NawiaMonitor

`CombatTelemetryServer` subskrybuje `CombatEventBus` i wysyla eventy jako
NDJSON po `127.0.0.1:19777`. Osobna aplikacja PyQt w katalogu `NawiaMonitor`
odbiera te wiadomosci i pokazuje je w panelach debugowych. Szczegoly sa w
`docs/ET systems/NawiaMonitor_README.md`.

`AgentPerceptionSystem` korzysta z tego samego `CombatEventBus`, aby dolaczac
do snapshotow agentow istotne eventy z ostatniego okna czasu.
