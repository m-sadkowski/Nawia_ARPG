# Map Pings

## Cel

`MapPingManager` przechowuje krotkotrwale pingi taktyczne stawiane na mapie.
Ping nie jest encja swiata: nie ma kolizji, AI targetowania ani logiki walki.
To lekki sygnal komunikacyjny, ktory moze zostac wykorzystany przez przyszly
fake multiplayer jako "ally/player marked this place".

Sa dwa typy pingow:

- `Info` - niebieski ping informacyjny,
- `Threat` - czerwony ping zagrozenia.

W kontekscie pracy inzynierskiej jest to infrastruktura wejscia dla agentow,
a nie algorytm wieloagentowy. Agent moze pozniej zdecydowac, czy ping oznacza
focus target, miejsce do zebrania sie, unikanie hazardu albo priorytet ruchu.

## Glowny kod

- `src/core/game/ping/MapPingManager.h`
- `src/core/game/ping/MapPingManager.cpp`
- `src/core/system/renderer/WorldAreaIndicator.h`
- `src/core/system/renderer/WorldAreaIndicator.cpp`
- `src/core/system/input/PlayerController.cpp`
- `src/core/game/agent/AgentPerceptionSystem.cpp`
- `src/core/game/telemetry/CombatTelemetryServer.cpp`

## Dzialanie

- gracz stawia aktualnie wybrany ping srodkowym przyciskiem myszy,
- `2` wybiera ping `Info`,
- `3` wybiera ping `Threat`,
- scroll przelacza aktualnie wybrany typ pingu,
- wybrany typ pingu jest widoczny jako mala niebieska/czerwona kropka nad jedzeniem w HUD,
- pozycja pingu jest liczona z raycastu do mapy, gdy mapa jest dostepna,
- ping jest widoczny w swiecie przez domyslnie 5 sekund,
- jednoczesnie moze istniec po jednym aktywnym pingu kazdego typu na zrodlo,
- `MapPingManager` pamieta ostatni ping kazdego zrodla i typu,
- pingi sa czyszczone przy zmianie poziomu.

Ping moze utworzyc tylko zrodlo typu `Player` albo `Ally`, ktore jest aktualnie
widoczne dla percepcji. To przygotowuje system pod przyszle ally bez mieszania
pingow przeciwnikow z komunikacja druzyny.

## Najwazniejsze funkcje

### `MapPingManager::placePing(...)`

Tworzy nowy ping w pozycji swiata i zapamietuje snapshot zrodla: `entity_id`,
nazwe, typ encji i frakcje. Wariant bez jawnego typu uzywa aktualnie wybranego
typu pingu. Zwraca pusty ping, jesli zrodlo nie moze pingowac.

Jesli zrodlo ma juz aktywny ping tego samego typu, nowy ping go zastapi.

### `MapPingManager::update(...)`

Aktualizuje wiek aktywnych pingow i usuwa te, ktorych czas widocznosci minal.
Domyslny czas widocznosci to 5 sekund.

### `MapPingManager::getActivePings()`

Zwraca pingi nadal widoczne na mapie. Agent Perception publikuje je jako
`visible_pings`.

### `MapPingManager::getRememberedPings()`

Zwraca ostatni ping dla kazdego zrodla i typu, rowniez po wygasnieciu
widocznosci. Agent Perception publikuje je jako `remembered_pings`.

### `MapPingManager::selectType(...)`

Ustawia aktywny typ pingu uzywany przez srodkowy przycisk myszy.

### `MapPingManager::cycleSelectedType(...)`

Przelacza aktywny typ pingu miedzy `Info` i `Threat`. Jest uzywane przez
scroll.

### `MapPingManager::render(...)`

Rysuje prosty marker 3D nad miejscem pingu. Marker zanika wraz z wiekiem pingu.
Podstawa pingu korzysta z `drawSoftGroundDisc(...)`, czyli gladkiego,
wypelnionego dysku bez kanciastego `DrawCylinderWires()`.

## Agent Perception

`AgentPerceptionSystem` dolacza pingi do snapshotow gracza i ally-side agentow:

- `visible_pings` - aktywne pingi widoczne przez 5 sekund,
- `remembered_pings` - ostatni znany ping od kazdego zrodla i typu.

Pingi sa traktowane jako komunikacja druzyny, nie jako FOV/raycast. Enemy nie
dostaja pingow player/ally-side w swoich snapshotach.

## Telemetria

Pingi trafiaja do `nawia.telemetry.agent_perception.v1` jako JSON:

```json
{
  "visible_pings": [
    {
      "id": 1,
      "ping_type": "Info",
      "age_seconds": 0.2,
      "duration_seconds": 5.0,
      "active": true,
      "position": {"x": 1.0, "y": 0.0, "z": 2.0},
      "source": {"valid": true, "entity_id": 1, "name": "Player", "entity_type": "Player", "faction": "Player"}
    }
  ],
  "remembered_pings": []
}
```

`NawiaMonitor` pokazuje liczbe aktywnych pingow `Info`, liczbe aktywnych
pingow `Threat` i liczbe zapamietanych pingow w zakladce `Agent Perception`.
Pelne dane pingu sa widoczne w zakladce `Pings` dla wybranego agenta oraz w
panelu JSON po zaznaczeniu wiersza agenta.
