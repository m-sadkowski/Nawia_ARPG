# Przewodnik po Ability

Ten dokument opisuje aktualny system umiejetnosci: statystyki z JSON, cooldown, typ celowania i tworzenie efektow w swiecie.

## Glowny model

`Ability` jest obiektem logicznym przypietym do encji przez `Entity::addAbility(...)`.

Umiejetnosc:

- trzyma nazwe, ikone i `AbilityStats`,
- pilnuje cooldownu,
- zna typ celowania,
- po uzyciu tworzy efekt, pocisk albo wpis do pending spawn.

Najwazniejsze pliki:

- `src/entity/abilities/Ability.h`
- `src/entity/abilities/AbilityStats.h`
- `src/entity/abilities/AbilityEffect.h`
- `src/entity/abilities/projectiles/ProjectileAbility.h`

## Statystyki

Statystyki sa w `assets/data/abilities.json` i sa wczytywane przez:

```cpp
const AbilityStats stats = Entity::getAbilityStatsFromJson("SwordSlash");
```

Najczestsze pola:

- `damage` - bazowe obrazenia,
- `cooldown` - czas odnowienia,
- `cast_range` - maksymalny zasieg uzycia,
- `hitbox_radius` - promien lub rozmiar obszaru trafienia,
- `duration` - czas zycia efektu,
- `projectile_speed` - predkosc pocisku.

## Typ celowania

`AbilityTargetType` opisuje, jak caller ma interpretowac cel:

- `POINT` - punkt swiata,
- `UNIT` - konkretna encja,
- `SELF` - zrodlo uzycia.

Typ powinien opisywac kontrakt `cast(...)`, nie tylko aktualny wizual. Pocisk lecacy do punktu powinien miec `POINT`, nawet jesli target pochodzi od kliknietej encji.

## Kontrakt `cast`

`cast(float target_x, float target_y)` zwraca `AbilitySpawn`, czyli `std::shared_ptr<Entity>`.

Zasady:

1. Na poczatku wywolaj `beginCast()`.
2. Jesli efekt powstaje od razu, zwroc utworzona encje.
3. Jesli efekt powstaje z opoznieniem animacji, zwroc `nullptr` i dodaj efekt przez `_caster->addPendingSpawn(...)`.
4. Nie dodawaj encji bezposrednio do `EntityManager`.

`beginCast()` sprawdza `_caster`, cooldown i uruchamia odnowienie. Dzieki temu klasy ability nie duplikuja guardow.

## Pociski

Dla pociskow dziedzicz po `ProjectileAbility`.

Ta klasa trzyma wspolne dane:

- nazwe pocisku,
- model i skale,
- teksture trafienia,
- ikone,
- offset kierunku modelu.

`ProjectileAbility::cast(...)` tworzy `Projectile`, ktory leci w strone celu i filtruje cele po frakcji.

## Melee i opoznione efekty

`SwordSlashAbility` jest aktualnym wzorcem melee:

- od razu obraca castera w strone celu,
- odpala animacje ataku,
- zapisuje dane opoznionego spawnu,
- w `update(...)` tworzy `SwordSlashEffect` w odpowiedniej fazie animacji.

Ten pattern stosuj dla atakow, gdzie hitbox ma pojawic sie dopiero w momencie impaktu.

## Pending spawns

Efekty nie powinny modyfikowac listy encji podczas iteracji po swiecie.

Aktualny przeplyw:

1. Ability zwraca efekt albo dodaje go do pending spawn castera.
2. `Engine` po update zbiera pending spawns z encji.
3. Nowe encje trafiaja do `EntityManager` po bezpiecznej stronie petli.

To samo dotyczy efektow wtornych, np. `ProjectileHitEffect`.

## Dobre praktyki

- Do celowania w walce uzywaj `getCenter()`, gdy liczy sie wysokosc modelu.
- Ability nie powinno znac `EntityManager`.
- Cooldown uruchamiaj przez `beginCast()` albo `startCooldown()`.
- Filtr friendly fire trzymaj w efekcie/pocisku, bo to on zna realny kontakt.
- Ikony ability laduj przez `ResourceManager`, tak jak inne tekstury UI.
