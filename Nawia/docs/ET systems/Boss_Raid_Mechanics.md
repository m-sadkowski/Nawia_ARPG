# Boss Raid Mechanics

## Cel

Ten system dodaje minimalny raidowy fundament dla bossow: cast metadata,
telegraphy i hazardy obszarowe. To nadal nie jest algorytm pracy
inzynierskiej. Jest to srodowisko testowe, na ktorym przyszly system
wieloagentowy bedzie mogl podejmowac decyzje typu: uniknij AoE, poczekaj na
koniec castu, rozpoznaj aktywna strefe zagrozenia.

## Glowny kod

- `src/entity/Entity.h`
- `src/entity/effects/BossTelegraphHazard.h`
- `src/entity/effects/BossTelegraphHazard.cpp`
- `src/core/system/renderer/WorldAreaIndicator.h`
- `src/core/system/renderer/WorldAreaIndicator.cpp`
- `src/core/game/EntityManager.cpp`
- `src/core/game/agent/AgentPerceptionSystem.h`
- `src/core/game/agent/AgentPerceptionSystem.cpp`
- `src/entity/actors/enemies/devil/Devil.cpp`
- `src/entity/actors/enemies/Frog.cpp`

## Cast metadata

Kazda encja moze wystawic lekki stan castu przez:

- `Entity::beginCastTelemetry(name, duration_seconds, interruptible)`
- `Entity::clearCastTelemetry()`
- `Entity::getCastState()`

To jest telemetryka i metadata decyzyjne, a nie system wykonywania ability.
Boss sam nadal steruje animacja i momentem odpalenia mechaniki.

`AgentPerceptionSystem` publikuje dla encji pola:

- `casting`
- `cast_name`
- `cast_duration`
- `cast_remaining`
- `cast_interruptible`

## BossTelegraphHazard

`BossTelegraphHazard` jest encja swiata typu `Hazard`. Ma dwie fazy:

- `Warning` - obszar jest widoczny, ale jeszcze nie zadaje obrazen.
- `Active` - obszar zadaje tick damage celom wrogiej frakcji.

Hazard nie blokuje ruchu i nie zmienia navmesha. Agent ma go zobaczyc w
percepcji i zdecydowac, czy oraz jak go ominac.

Wizualnie hazard uzywa `drawSoftGroundDisc(...)`: gladkiego, wypelnionego
dysku bez wireframe'owych krawedzi. Dzieki temu AoE nie wyglada jak zestaw
kanciastych prostokatow z kamery izometrycznej.

Najwazniejsze pola:

- `radius`
- `warning_seconds`
- `active_seconds`
- `damage_per_tick`
- `tick_interval`
- `root_seconds_on_hit`
- `source_context`

## Wpiecie w bossy

### Bies / Devil

Bossowy `Devil` rozpoznawany jest po wysokim `max_hp` albo nazwie `Bies` /
`Devil Lord`. Dash ma teraz cast telemetry `Devil Dash`. W czasie przygotowania
spawnuje `Dash Impact` jako warning hazard na zablokowanej pozycji celu. Po
dotarciu zostawia krotki `Scorched Ground`.

### Ropuch / Frog

Bossowy `Frog` rozpoznawany jest po `max_hp >= 200`. Istniejacy jezyk dostal
cast telemetry `Tongue Strike`. Ropuch dostal tez nowa mechanike `Toxic Pool`:
castowany telegraph pod celem, ktory po chwili zmienia sie w aktywna
trujaco-lepka kaluze.

## Telemetria i percepcja

Hazardy sa widoczne w `observed_entities` jako `entity_type = Hazard`.
Dodatkowe pola hazardu:

- `hazard`
- `hazard_phase`
- `hazard_radius`
- `hazard_time_to_activate`
- `hazard_remaining`
- `hazard_damage_per_tick`
- `hazard_tick_interval`
- `hazard_source_entity_id`

Snapshot agenta zawiera tez `nearby_hazard_count`.

NawiaMonitor pokazuje hazardy w `Seen Entities`, a w kolumnie `Flags` wyswietla
stan castu i hazardu.
