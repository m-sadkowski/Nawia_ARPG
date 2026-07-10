# Agent Perception

## Cel

`AgentPerceptionSystem` buduje snapshot lokalnego stanu swiata dla narzedzi ET,
telemetryki i testow runtime. Nie podejmuje decyzji, nie liczy priorytetow, nie
wybiera roli i nie wykonuje komend.

W kontekscie pracy inzynierskiej ten system jest fundamentem infrastruktury,
a nie algorytmem zachowania. Przygotowuje ustrukturyzowane dane wejsciowe,
ktore mozna analizowac w dokumentacji, monitorze albo zewnetrznych testach,
bez dodawania do gry nowych elementow decyzyjnych.

Aktualny przeplyw:

```text
EntityManager + CombatEventBus + MapPingManager -> AgentPerceptionSystem -> snapshots -> NawiaMonitor
```

## Glowny kod

- `src/core/game/agent/AgentPerceptionSystem.h`
- `src/core/game/agent/AgentPerceptionSystem.cpp` - orkiestracja update'u,
  ustawienia i lookup snapshotow.
- `src/core/game/agent/AgentPerceptionSnapshot.cpp` - budowanie snapshotu,
  obserwowane encje, ability i hazard metadata.
- `src/core/game/agent/AgentPerceptionMemory.cpp` - pamiec `lost_entities`.
- `src/core/game/agent/AgentPerceptionRules.cpp` - kandydaci, widocznosc,
  relacje, pingi i powody znikniecia.
- `src/core/game/agent/AgentPerceptionSupport.h` - klasyfikacja typow encji i
  wspolne identity helpery.
- `src/core/game/agent/AgentSystemMath.h` - wspolna matematyka odleglosci.
- `src/core/Engine.cpp`
- `src/core/game/telemetry/CombatTelemetryServer.cpp`
- `docs/ET systems/Boss_Raid_Mechanics.md`
- `docs/ET systems/Entity_Identity_and_Damage_Context.md`
- `NawiaMonitor/nawia_monitor/main_window.py`

## Co zawiera snapshot

Kazdy `AgentPerceptionSnapshot` opisuje jednego kandydata na agenta.
Domyslnie snapshoty sa budowane dla encji typu `Player`, `Ally` i `Enemy`.

Snapshot zawiera:

- `self` - stan obserwujacej encji w C++,
- `agent` - ten sam stan obserwujacej encji w JSON telemetryki,
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
`entity_id`, pozycje, HP, flagi `dormant` i `visible`, oraz podstawowe metadata
interakcji. Encje martwe, umierajace, dormant albo niewidoczne dla percepcji
nie trafiaja do listy aktualnie widzianych.

Encje moga tez publikowac cast metadata:

- `casting`
- `cast_name`
- `cast_duration`
- `cast_remaining`
- `cast_interruptible`

Hazardy bossow sa obserwowane jako `entity_type = Hazard`. Ich snapshot zawiera
faze `Warning` albo `Active`, promien, czas do aktywacji, pozostaly czas
aktywny i tick damage. Dzieki temu agent moze rozrozniac obszar, ktory dopiero
bedzie niebezpieczny, od obszaru, ktory juz zadaje obrazenia.

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

`observed_entities` w telemetryce moze byc przyciete limitem monitora, ale
pamiec `lost_entities` jest aktualizowana z pelnego zestawu encji widzianych
w promieniu percepcji. Dzieki temu agent nie "gubi" encji tylko dlatego, ze
nie zmiescila sie w eksportowanej tabeli najblizszych obserwacji.

`lost_entities` przechowuje ostatnia znana pozycje, czas od ostatniego
widzenia i powod znikniecia, np. `OutOfRange`, `Dormant` albo `NotVisible`.
Zwykla pamiec utraconych encji domyslnie trwa 60 sekund. Encje potwierdzone
jako martwe, umierajace albo usuniete ze swiata sa trzymane tylko jako krotki
terminalny slad diagnostyczny, domyslnie 4 sekundy, z powodem `Dead`, `Dying`
albo `Removed`. Pamiec jest indeksowana po `entity_id`, wiec rozroznia
konkretne instancje tego samego typu.

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

Zwraca aktualny zestaw snapshotow. To jest glowne wejscie dla monitora,
telemetryki i zewnetrznych testow.

### `AgentPerceptionSystem::findSnapshot(...)`

Pozwala pobrac snapshot konkretnej encji po `EntityId` albo referencji do
encji.

### `AgentPerceptionSystem::setSettings(...)`

Pozwala zmienic promien percepcji, okno pamieci eventow i limity list.
Ustawienia obejmuja tez czas pamieci `lost_entities`, czas krotkiego sladu
terminalnego oraz wlaczanie NPC, neutralnych obiektow i pociskow.

## Telemetria

`CombatTelemetryServer` wysyla snapshoty jako NDJSON z typem:

```json
{"schema": "nawia.telemetry.agent_perception.v1"}
```

Wiadomosci sa throttlowane w `Engine` do ok. 4 razy na sekunde, zeby monitor
nie dostawal pelnego snapshotu co klatke.

Encje w JSON maja pole `entity_id`. `runtime_id` jest chwilowym aliasem tej
samej wartosci dla kompatybilnosci.

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

Dropdown `Agent` filtruje tabele do wszystkich agentow albo jednego
konkretnego agenta. Klikniecie wiersza wybiera agenta do szczegolowego
podgladu. To jest wybor punktu widzenia: snapshot pokazuje, co widzi i pamieta
wskazana encja.

Pod glowna tabela sa szczegolowe zakladki dla wybranego agenta:

- `Seen Entities` - pelna lista aktualnie widzianych encji z typem, frakcja,
  relacja, HP, dystansem, pozycja, flagami stanu i dostepnoscia interakcji,
- `Lost Memory` - encje, ktore agent widzial wczesniej, z ostatnia znana
  pozycja, czasem od utraty i powodem znikniecia,
- `Combat` - istotne eventy walki z kierunkiem `incoming`, `outgoing` albo
  `nearby`; naglowek pokazuje ostatnie obrazenia otrzymane przez agenta,
- `Pings` - aktywne i zapamietane pingi `Info`/`Threat`,
- `Abilities` - ability agenta, gotowosc, cooldown, zasieg i obrazenia.

Prawy panel nadal pokazuje pelny JSON zaznaczonego eventu albo snapshotu, ale
nie jest juz jedynym sposobem sprawdzania percepcji.

Zakladka jest jedna, ale zawiera osobny wiersz dla kazdego aktywnego agenta.
Na razie kandydatami na agentow sa `Player`, `Ally` i `Enemy`. NPC sa
obserwowalne, ale nie dostaja wlasnych snapshotow jako kandydaci.

## Granice systemu

Ten system nie decyduje:

- gdzie agent ma isc,
- kogo ma atakowac,
- czy powinien uniknac AoE,
- jak rozdzielic role,
- jak liczyc aggro,
- czy zachowanie bylo poprawne.

Te elementy naleza do osobnych narzedzi, testow albo warstw badawczych poza
`AgentPerceptionSystem`.

## Uwagi wydajnosciowe

Percepcja jest liczona jako infrastruktura debugowo-telemetryczna, a nie jako
system renderingu. Aktualnie buduje snapshoty dla aktywnych kandydatow na
snapshot i uzywa indeksu `entity_id -> entity` w ramach pojedynczego update'u.
Przy wiekszej liczbie agentow kolejnym krokiem bedzie ograniczenie czestotliwosci
symulacyjnej percepcji i/lub spatial query zamiast pelnego skanowania encji.
