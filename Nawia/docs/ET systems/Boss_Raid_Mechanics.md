# Boss Raid Mechanics

## Cel

Ten system dodaje minimalny raidowy fundament dla bossow: cast metadata,
telegraphy i hazardy obszarowe. To nadal nie jest algorytm pracy
inzynierskiej. Jest to srodowisko runtime, ktore wystawia czytelne fakty do
percepcji, telemetryki i testow.

## Glowny kod

- `src/entity/Entity.h`
- `src/entity/effects/BossTelegraphHazard.h`
- `src/entity/effects/BossTelegraphHazard.cpp`
- `src/core/system/renderer/WorldAreaIndicator.h`
- `src/core/system/renderer/WorldAreaIndicator.cpp`
- `src/core/game/EntityManager.cpp`
- `src/core/game/agent/AgentPerceptionSystem.h`
- `src/core/game/agent/AgentPerceptionSystem.cpp`
- `src/core/game/agent/AgentPerceptionSnapshot.cpp`
- `src/entity/actors/enemies/devil/Devil.cpp`
- `src/entity/actors/enemies/rift_binder/RiftBinder.cpp`
- `src/entity/actors/enemies/RiftTotem.cpp`
- `src/entity/effects/FireRainHazard.cpp`
- `src/entity/actors/enemies/frog/Frog.cpp`

## Cast metadata

Kazda encja moze wystawic lekki stan castu przez:

- `Entity::beginCastTelemetry(name, duration_seconds, interruptible)`
- `Entity::clearCastTelemetry()`
- `Entity::getCastState()`

To jest telemetryka i metadata runtime, a nie system wykonywania ability.
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

Hazard nie blokuje ruchu i nie zmienia navmesha. Jest widoczny w percepcji i
telemetryce jako fakt o swiecie.

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
- `knock_down_player_on_hit`
- `expanding_wave`
- `wave_speed`
- `wave_width`
- `source_context`

## Wpiecie w bossy

### Bies / Devil

Bossowy `Devil` rozpoznawany jest po wysokim `max_hp` albo nazwie `Bies` /
`Devil Lord`. Dash ma teraz cast telemetry `Devil Dash`. W czasie przygotowania
spawnuje `Dash Impact` jako warning marker na zablokowanej pozycji celu. Ten
marker nie zadaje obrazen. Dopiero po zakonczeniu ruchu dasha boss zostawia
krotki, aktywny `Scorched Ground`.

Bossowy wariant nie zawsze wybiera dash. Gdy cel jest w srednim dystansie,
moze zamiast tego uzyc `Ground Slam`: szybki wizualny podskok na animacji
`idle`, warning maksymalnego zasiegu pod bossem i aktywna fala uderzeniowa.
Fala losuje promien z zakresu 4-7 jednostek, rozchodzi sie od Biesa z
predkoscia bazowego ruchu gracza i powala dopiero wtedy, gdy ring przetnie
pozycje gracza.

### Ropuch / Frog

Bossowy `Frog` rozpoznawany jest po `max_hp >= 200`. Istniejacy jezyk dostal
cast telemetry `Tongue Strike`. Ropuch dostal tez nowa mechanike `Toxic Pool`:
castowany telegraph pod celem, ktory po chwili zmienia sie w aktywna
trujaco-lepka kaluze.

### Siewca Chaosu / Dragon

`RiftBinder` pozostaje techniczna nazwa implementacji, ale boss w danych gry
wystepuje jako `Dragon` i uzywa modelu `assets/models/actors/dragon/dragon.glb`.
Widoczna nazwa walki to `Siewca Chaosu`. Boss ma cztery stage'e totemowe:
na start spawnuje 3 totemy, potem przy progach HP 75/50/25% spawnuje kolejno
4, 5 i 7 totemow.
Dopoki stage'owe totemy zyja, boss ignoruje obrazenia, wiec gracz musi niszczyc
cele pomocnicze zanim moze dalej bic bossa.

`RiftTotem` jest encja typu Enemy, wiec da sie go targetowac i niszczyc zwyklymi
atakami. Kazdy totem renderuje model `assets/models/totem.glb`. Kazdy totem
moze przywolac jednego pomocnika `Sluga Totemu`. Pomocnik uzywa modelu
`assets/models/actors/walking_dead/walking_dead_2.glb`, ale animacje zostaja z
bazowego Walking Dead. Te assety sa czescia implementacji encountera, a nie
per-boss konfiguracja JSON.

W czasie oslony totemowej boss nie stoi bezczynnie: castuje `Stone Volley`,
losowo teleportuje sie przez `Dragon Blink` i odpala `Fire Rain`. `Fire Rain`
jest aktywny tylko dopoki zyje przynajmniej jeden totem. Cooldown maleje wraz z
kolejnymi stage'ami i zbijaniem totemow w aktualnym stage'u: 10.0-8.5s,
8.0-6.5s, 6.5-5.5s, 5.5-4.0s. `Stone Volley`
po zakonczeniu castu tworzy trzy mniejsze encje `Projectile`, czyli zachowuje
sie podobnie do fireballa. Aktualnie uzywa `assets/models/fireball.glb` z
szarym tintem, dopoki nie powstanie dedykowany asset kamiennego pocisku.
`FireRainHazard` dziedziczy z `BossTelegraphHazard`, wiec jest widoczny dla
percepcji jak zwykly hazard, ale renderuje tez spadajace ogniste pociski nad
obszarem.

Mapowanie animacji smoka: `Death` dla smierci, `Fast_Flying` dla ruchu,
`Flying_Idle` dla postoju, `HitReact` po realnym otrzymaniu obrazen,
`Headbutt` dla `Fire Rain`, `Punch` dla `Stone Volley` i `No` przed
teleportacja `Dragon Blink`.

Ten boss jest przydatny jako testowy encounter ET, bo publikuje kilka rodzajow
faktow naraz: cele pomocnicze, addy, hazardy obszarowe, teleport bossa i stan
odpornosci na obrazenia.

## Telemetria i percepcja

Hazardy sa widoczne w `observed_entities` jako `entity_type = Hazard`.
Dodatkowe pola hazardu:

- `hazard`
- `hazard_phase`
- `hazard_radius`
- `hazard_current_radius`
- `hazard_time_to_activate`
- `hazard_remaining`
- `hazard_damage_per_tick`
- `hazard_tick_interval`
- `hazard_knock_down_player_on_hit`
- `hazard_expanding_wave`
- `hazard_source_entity_id`

Snapshot agenta zawiera tez `nearby_hazard_count`.

NawiaMonitor pokazuje hazardy w `Seen Entities`, a w kolumnie `Flags` wyswietla
stan castu i hazardu.
