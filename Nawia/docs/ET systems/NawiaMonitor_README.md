# NawiaMonitor

## Cel

`NawiaMonitor` to osobna aplikacja PyQt do podgladu danych potrzebnych przy
Engineering Thesis. Gra pozostaje zrodlem prawdy, a monitor tylko odbiera i
wyswietla telemetrie.

Aktualny przeplyw:

```text
CombatEventBus -> CombatTelemetryServer -> TCP 127.0.0.1:19777 -> NawiaMonitor
```

## Pliki

- `Nawia/src/core/game/telemetry/CombatTelemetryServer.h`
- `Nawia/src/core/game/telemetry/CombatTelemetryServer.cpp`
- `NawiaMonitor/run_monitor.py`
- `NawiaMonitor/nawia_monitor/telemetry_client.py`
- `NawiaMonitor/nawia_monitor/main_window.py`

## Uruchomienie monitora

```powershell
cd C:\Users\msadk\Documents\GitHub\Nawia_ARPG\NawiaMonitor
python -m venv .venv
.\.venv\Scripts\pip install -r requirements.txt
.\.venv\Scripts\python run_monitor.py
```

Monitor moze byc uruchomiony przed gra albo po niej. Klient PyQt automatycznie
probuje laczyc sie ponownie z `127.0.0.1:19777`.

## Uruchomienie gry z poprawnym srodowiskiem VS

```powershell
& $env:ComSpec /d /s /c 'call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build "Nawia\out\build\x64-Release" --config Release'
```

## Protokol

Transport uzywa NDJSON: jeden event to jedna linia JSON zakonczona `\n`.

Przykladowy event:

```json
{
  "schema": "nawia.telemetry.combat.v1",
  "sequence_id": 42,
  "time_seconds": 12.5,
  "event_type": "DamageDealt",
  "source": {"valid": true, "name": "Player", "entity_type": "Player"},
  "target": {"valid": true, "name": "Spider", "entity_type": "Enemy"},
  "source_label": "Sword Slash",
  "amount": 17,
  "hp_before": 40,
  "hp_after": 23,
  "lethal": false
}
```

## Aktualne panele

- status polaczenia,
- licznik eventow,
- licznik `DamageDealt`, `EntityKilled`, `AbilityCastStarted`,
- tabela eventow,
- tabela `Agent Perception` z aktualnie widzianymi i utraconymi encjami oraz pingami `Info`/`Threat`,
- surowy JSON zaznaczonego eventu.

## Miejsca rozbudowy

Monitor jest przygotowany pod kolejne panele:

- Agent Perception,
- map pings,
- stan agentow,
- decision tree / behavior tree,
- przyszly GOAP planner,
- boss phase timeline,
- threat table,
- replay lub eksport danych do analizy.

## Granice systemu

`CombatTelemetryServer` dziala best-effort. Jesli monitor nie jest wlaczony
albo polaczenie zostanie zerwane, gra nadal dziala normalnie. Telemetria nie
moze sterowac logika walki ani zmieniac wyniku symulacji.
