# Kreator leveli

`Kreator leveli` to specjalny level do ukladania lokacji, map, spawn pointow i
obiektow bezposrednio w grze.

## Uruchomienie

W menu wyboru levelu wybierz `Kreator leveli`. Edytor startuje na pustej
lokacji z placeholderem mapy.

## Lewy panel

Lewy panel ustawia mape aktualnej lokacji:

- model mapy,
- skale,
- offset,
- obrot,
- minimalna wysokosc NavMesh,
- spawn gracza przez `Ustaw spawn`,
- przeladowanie mapy przez `Przeladuj`.

## Srodkowy panel

Srodkowy panel wybiera albo tworzy lokacje. `Zapisz` tworzy dwa pliki:

```text
assets/data/locations/<lokacja>.json
assets/data/locations/objects_<lokacja>.json
```

Przy nadpisywaniu istniejacych plikow edytor pyta o potwierdzenie.

## Prawy panel

Prawy panel dodaje obiekty w miejscu, w ktorym stoi gracz:

- spawner przeciwnikow,
- skrzynie z lootem, opcjonalnym zamkiem i `key_id`,
- NPC,
- prop,
- teleport,
- checkpoint,
- boss trigger.
- story trigger, story anchor i hub zielarza dla sekwencji fabularnych.

Teleport wybiera lokacje docelowa z listy istniejacych lokacji. Boss trigger
wybiera id bossa z `assets/data/bosses.json`.

Dla lokacji Wczory kreator obsluguje dedykowane pliki oswietlenia:

- `wczora.json` zapisuje i laduje `assets/maps/wczora_las_lighting.json`,
- `przedsionek_nawii.json` zapisuje i laduje
  `assets/maps/wczora_przedsionek_nawii_lighting.json`.

Pozostale lokacje uzywaja domyslnego `assets/maps/forest_lighting.json`, chyba
ze dodamy im osobne mapowanie w kodzie kreatora.

Skrzynia moze byc otwarta albo zamknieta. Dla zamknietej skrzyni podajesz
`key_id` recznie albo wybierasz item z bazy, a kreator pokazuje nazwe itemu
przy ID. Zapis trafia do `objects_*.json` jako `locked: true` i `key_id`.

## Story NPC i hub zielarza

W menu NPC sa dodatkowe presety fabularne:

- `Story Human` - ogolny NPC oparty o pola `model`, `animation_bundle`, `idle_animation`, `walk_animation`, `dialogue_key` i opcjonalna trase po dialogu.
- `Zielarz` - story NPC z modelem `assets/models/actors/herbalist/herbalist.glb` i domyslnym `dialogue_key: herbalist_placeholder`.
- `Cmentarz: female_warrior` i `Cmentarz: male_npc_1` - gotowe presety ocalonych z cmentarza. Maja domyslny model, indeksowane animacje `idle/walk`, dialog, trase do `Herbalist Hub` i checkpoint dojscia.
- `Forest Lost NPC` - grupa trzech NPC z lasu: `female_npc_2`, `male_npc_2` i `milena_sister`.

`HUB zielarza` zapisuje encje `herbalist_hub` z promieniem `radius`. Grupa
`Forest Lost NPC` szuka domyslnie encji o nazwie `Herbalist Hub`. Po dialogu
idzie do tego huba, opuszcza siostre Mileny na ziemie, odtwarza jej animacje
`Death` od konca jako wstawanie, a potem cala trojka rozchodzi sie ruchem do
losowych punktow w promieniu huba.

Domyslne parametry `Forest Lost NPC` zapisywane przez kreator:

```json
{
  "npc_class": "forest_lost_group",
  "hub_name": "Herbalist Hub",
  "dialogue_key": "forest_lost_group",
  "checkpoint_on_arrival": "forest_lost_group_arrived",
  "sister_carry_height": 1.6,
  "sister_drop_duration": 0.6,
  "male_carry_spacing_multiplier": 2.4,
  "death_animation_index": 0,
  "idle_animation_index": 9,
  "walk_animation_index": 16,
  "walk_back_animation_index": 17
}
```

Najczesciej strojone pola:

- `sister_carry_height` - wysokosc niesionego ciala siostry Mileny.
- `male_carry_spacing_multiplier` - odsuniecie tylnego noszacego wzgledem kierunku marszu.
- `sister_drop_duration` - czas opuszczania siostry na ziemie przed wstawaniem.
- `checkpoint_on_arrival` - checkpoint wysylany dopiero po tym, jak cala trojka dojdzie do losowych miejsc w hubie.

## Wczora: minimalna konfiguracja testowa

Dla aktualnego flow nie trzeba wypelniac wszystkich pol kreatora. Wystarcza:

- `HUB zielarza`: nazwa `Herbalist Hub`, radius wedlug obszaru przy chacie.
- `Zielarz`: zostaw domyslne pola presetu.
- `Cmentarz: female_warrior`: zostaw domyslne pola presetu.
- `Cmentarz: male_npc_1`: zostaw domyslne pola presetu.
- `Forest Lost NPC`: zostaw domyslne pola presetu.
- `Story Trigger` przy Czarownicy przed wejsciem do Nawii:
  `dialogue_key = witch_to_nawia`, `target_location = Przedsionek Nawii`,
  `checkpoint = witch_to_nawia`.
- `Story Trigger` po powrocie z Nawii:
  `dialogue_key = witch_after_bies_placeholder`,
  `condition boss defeated = bies`, `checkpoint = witch_after_bies`,
  bez target location.

W `Przedsionek Nawii` jest juz zapisany `boss_trigger` na `bies` i teleport
powrotny do `Dolina Nedzy` odblokowywany po pokonaniu Biesa.

## Czarownica

Spawner `Czarownica` tworzy enemy `witch`. Boss `czarownica` w
`assets/data/bosses.json` ma `enemy_type: Witch`. Model:
`assets/models/actors/witch/witch.glb`.

Animacje:

- `0` death,
- `2` otrzymanie obrazen,
- `5` idle,
- `7` strzal blyskawica, tymczasowo maly fireball,
- `10` przywolanie pomagiera,
- `16` bieg.

AI Czarownicy trzyma dystans, strzela malymi fireballami, a po otrzymaniu
obrazen odgrywa hit, potem animacje strzalu bez pocisku, powala gracza i
przywoluje `WalkingDead`.

## Testowanie

`Testuj` tworzy runtime encje z aktualnie ulozonych obiektow. Po starcie testu
przycisk zmienia sie w `Zakoncz test`, ktory usuwa testowe encje i wraca do
edycji.

## Pliki w buildzie i repo

Kreator zapisuje wzgledem katalogu uruchomienia gry. Jezeli gra dziala z builda,
pliki moga powstac w:

```text
Nawia/out/build/x64-Release/assets/data/locations/
```

Zeby zapisac lokacje w repozytorium, przenies oba pliki do:

```text
Nawia/assets/data/locations/
```

Nastepny build skopiuje je obok `Nawia.exe`.
