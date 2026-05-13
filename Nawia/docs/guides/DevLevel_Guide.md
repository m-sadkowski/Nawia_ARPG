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

Teleport wybiera lokacje docelowa z listy istniejacych lokacji. Boss trigger
wybiera id bossa z `assets/data/bosses.json`.

Skrzynia moze byc otwarta albo zamknieta. Dla zamknietej skrzyni podajesz
`key_id` recznie albo wybierasz item z bazy, a kreator pokazuje nazwe itemu
przy ID. Zapis trafia do `objects_*.json` jako `locked: true` i `key_id`.

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
