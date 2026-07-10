# Agent System Guide

## Zakres

Agent system w tym repo jest infrastruktura ET, a nie logika zachowania.
Obejmuje dwa runtime adaptery:

- `AgentPerceptionSystem` - buduje read-only snapshoty swiata.
- `AgentCommandInterface` - wykonuje jawne komendy na encjach i zwraca status.

System nie dodaje nowych decyzji, rol, planowania, koordynacji ani zachowan.
Takie eksperymenty powinny byc oddzielone od runtime gry i komunikowac sie
przez istniejace snapshoty, komendy oraz telemetrie.

## Pliki

Kod znajduje sie w `src/core/game/agent`.

- `AgentPerceptionSystem.h/.cpp` - publiczny kontrakt, ustawienia i update.
- `AgentPerceptionSnapshot.cpp` - budowanie snapshotow encji, ability,
  interakcji i hazardow.
- `AgentPerceptionMemory.cpp` - utracone encje i powody znikniecia.
- `AgentPerceptionRules.cpp` - kandydaci, widocznosc, relacje i pingi.
- `AgentCommandInterface.h/.cpp` - publiczny kontrakt, submit/cancel i dispatch.
- `AgentCommandExecution.cpp` - wykonanie ruchu, ataku, ability i interakcji.
- `AgentCommandSupport.cpp` - pathing, lookup encji, stop i finalizacja komend.
- `AgentCommandStrings.cpp` - nazwy enumow dla telemetryki.
- `AgentSystemMath.h` - wspolne helpery dystansu.

## Zasady zmian

- Nie dodawac tutaj zachowan decyzyjnych.
- Nie mieszac lokalnego zachowania encji z adapterem komend.
- Nie query'owac calego swiata z narzedzi, jesli wystarcza snapshot percepcji.
- Nie ukrywac bledow runtime fallbackami; komenda ma konczyc sie jawnym
  statusem i `failure_reason`.
- Dokumentowac zmiany kontraktu w `docs/ET systems/Agent_Perception.md` albo
  `docs/ET systems/Agent_Command_Interface.md`.

## Telemetria

`Engine` publikuje snapshoty percepcji i stan komend przez
`CombatTelemetryServer`. NawiaMonitor powinien traktowac te wiadomosci jako
diagnostyke i dane do analizy, nie jako zrodlo nowej logiki gry.
