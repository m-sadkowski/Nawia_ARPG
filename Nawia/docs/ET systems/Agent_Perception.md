# Agent Perception

## Cel

`AgentPerceptionSystem` buduje snapshot tego, co agent moze wykorzystac jako
dane wejsciowe. Nie podejmuje decyzji, nie liczy threat table, nie wybiera roli
i nie wykonuje komend. To warstwa obserwacji dla przyszlych algorytmow pracy
inzynierskiej.

W kontekscie pracy inzynierskiej ten system jest fundamentem infrastruktury,
a nie glownym algorytmem wieloagentowym. Przygotowuje ustrukturyzowane dane
wejsciowe dla pozniejszych systemow decyzyjnych, takich jak koordynacja
agentow, wybor roli, planowanie akcji, threat reasoning albo fake multiplayer.
Wlasciwa czesc inzynierska zaczyna sie dopiero tam, gdzie agent wykorzystuje
te obserwacje do podejmowania decyzji i wspolpracy z innymi agentami.

Aktualny przeplyw:

```text
EntityManager + CombatEventBus + MapPingManager -> AgentPerceptionSystem -> snapshots -> NawiaMonitor
```

## Glowny kod

- `src/core/game/agent/AgentPerceptionSystem.h`
- `src/core/game/agent/AgentPerceptionSystem.cpp`
- `src/core/Engine.cpp`
- `src/core/game/telemetry/CombatTelemetryServer.cpp`
- `NawiaMonitor/nawia_monitor/main_window.py`

## Co zawiera snapshot

Kazdy `AgentPerceptionSnapshot` opisuje jednego kandydata na agenta.
Domyslnie snapshoty sa budowane dla encji typu `Player`, `Ally` i `Enemy`.

Snapshot zawiera:

- `self` - stan obserwujacej encji,
- `current_target` - aktualny target encji, jesli istnieje,
- `last_damage_source` - ostatni agresor zapamietany przez encje,
- `observed_entities` - aktualnie widziane encje w promieniu percepcji,
- `lost_entities` - encje widziane wczesniej, ktore zniknely z pola widzenia,
- `abilities` - stan ability, cooldowny i podstawowe metadata,
- `visible_pings` - aktywne pingi druzyny widoczne na mapie,
- `remembered_pings` - ostatni zapamietany ping od kazdego zrodla i typu,
- `recent_combat_events` - istotne eventy z ostatniego okna czasu,
- liczniki pobliskich wrogow, sojusznikow, NPC, neutralnych i pociskow.

Kazda encja w `observed_entities` ma typ, frakcje, relacje wobec obserwatora,
pozycje, HP, flagi `dormant` i `visible`, oraz podstawowe metadata
interakcji. Encje martwe, umierajace, dormant albo niewidoczne dla percepcji
nie trafiaja do listy aktualnie widzianych.

Metadata interakcji:

- `interactable` - czy encja implementuje interakcje,
- `interaction_available` - czy interakcja jest teraz mozliwa,
- `interaction_range` - zasieg interakcji,
- `interaction_state` - opis stanu, np. `Closed`, `Open`, `Locked`,
  `Available` albo `Unavailable`.

Skrzynki sa zapamietywane jako neutralne obiekty. Ich snapshot zawiera stan
otwarcia/zamkniecia, blokade i informacje, czy interakcja jest dostepna. NPC
sa zapamietywani jako neutralne encje obserwowalne; snapshot pokazuje, czy
`canInteract()` aktualnie pozwala rozpoczac interakcje.

`lost_entities` przechowuje ostatnia znana pozycje, czas od ostatniego
widzenia i powod znikniecia, np. `OutOfRange`, `Dormant` albo `NotVisible`.
Encje potwierdzone jako martwe albo usuniete ze swiata sa usuwane z pamieci.

Pingi z `MapPingManager` sa traktowane jako komunikacja druzyny, a nie jako
wynik FOV/raycast. Snapshoty player/ally-side dostaja aktywne pingi w
`visible_pings` i ostatnie znane pingi zrodel oraz typow w
`remembered_pings`. Snapshoty enemy nie dostaja player/ally-side pingow.

Kazdy ping ma `ping_type`: `Info` albo `Threat`.

## Najwazniejsze funkcje

### `AgentPerceptionSystem::update(...)`

Buduje nowe snapshoty na podstawie aktywnych encji z `EntityManager`, eventow
z `CombatEventBus` i pingow z `MapPingManager`. Jest wolane przez `Engine` po
update swiata.

### `AgentPerceptionSystem::getSnapshots()`

Zwraca aktualny zestaw snapshotow. To bedzie glowne wejscie dla przyszlych
systemow decyzyjnych agentow.

### `AgentPerceptionSystem::findSnapshot(...)`

Pozwala pobrac snapshot konkretnej encji po runtime id albo referencji do
encji.

### `AgentPerceptionSystem::setSettings(...)`

Pozwala zmienic promien percepcji, okno pamieci eventow i limity list.
Ustawienia obejmuja tez czas pamieci `lost_entities`, oraz wlaczanie NPC,
neutralnych obiektow i pociskow.

## Telemetria

`CombatTelemetryServer` wysyla snapshoty jako NDJSON z typem:

```json
{"schema": "nawia.telemetry.agent_perception.v1"}
```

Wiadomosci sa throttlowane w `Engine` do ok. 4 razy na sekunde, zeby monitor
nie dostawal pelnego snapshotu co klatke.

## NawiaMonitor

Monitor ma osobna zakladke `Agent Perception`. Pokazuje:

- frame,
- czas,
- agenta,
- HP,
- czy agent jest widoczny dla percepcji,
- aktualny target,
- liczbe aktualnie widzianych encji,
- liczbe encji w pamieci `lost_entities`,
- liczbe aktywnych pingow informacyjnych,
- liczbe aktywnych pingow zagrozenia,
- liczbe zapamietanych pingow,
- liczbe pobliskich wrogow i sojusznikow,
- liczbe pobliskich NPC,
- liczbe istotnych eventow,
- stan interakcji agenta, jesli dany wiersz reprezentuje obiekt interaktywny,
- gotowe ability.

Po zaznaczeniu wiersza panel szczegolow pokazuje pelny JSON snapshotu.

Zakladka jest jedna, ale zawiera osobny wiersz dla kazdego aktywnego agenta.
Na razie kandydatami na agentow sa `Player`, `Ally` i `Enemy`. NPC sa
obserwowalne, ale nie dostaja wlasnych snapshotow decyzyjnych.

## Granice systemu

Ten system nie decyduje:

- gdzie agent ma isc,
- kogo ma atakowac,
- czy powinien uniknac AoE,
- jak rozdzielic role,
- jak liczyc aggro,
- czy zachowanie bylo poprawne.

Te elementy naleza do kolejnych warstw: Agent Command Interface, walidacji,
threat table, GOAP albo innego algorytmu decyzyjnego.
