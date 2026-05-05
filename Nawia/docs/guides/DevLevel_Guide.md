# Przewodnik po DevLevel

`DevLevel` jest roboczym poziomem do szybkiego strojenia propsow i swiatla.

## Do czego sluzy

Mozesz:

1. Kliknac PPM na mape i zapisac pozycje propa.
2. Przesuwac glowne swiatlo klawiszami.
3. Zapisac konfiguracje swiatla do JSON.

## Dodawanie propa

1. Kliknij PPM na mapie.
2. Wpisz nazwe obiektu.
3. Nacisnij `Enter`, aby zapisac.
4. Nacisnij `Esc`, aby anulowac.

Zapis trafia do:

```text
assets/data/static_objects_dev.json
```

Przyklad wpisu:

```json
{
    "name": "tree",
    "x": 12.5,
    "y": -4.0
}
```

Pozycje sa w logicznej przestrzeni X/Y mapy, czyli w 3D odpowiadaja X/Z.

## Sterowanie swiatlem

- `Arrow Up/Down` przesuwa swiatlo po osi Z.
- `Arrow Left/Right` przesuwa swiatlo po osi X.
- `Page Up/Page Down` przesuwa swiatlo po osi Y.
- `S` zapisuje konfiguracje.

Zapis trafia do:

```text
assets/maps/forest_lighting.json
```

## Format lighting JSON

```json
{
    "ambient": [25, 25, 25, 255],
    "lights": [
        {
            "type": 0,
            "enabled": 1,
            "position": [-50.0, 50.0, -50.0],
            "target": [0.0, 0.0, 0.0],
            "color": [255, 255, 255, 255]
        }
    ]
}
```

## Workflow

1. Uruchom `DevLevel`.
2. Ustaw swiatlo tak, zeby scena byla czytelna.
3. Zapisz `S`.
4. Rozstaw propy przez PPM.
5. Przenies dane do docelowego levelu albo pliku spawn.

## Uwagi

- `DevLevel` jest narzedziem roboczym, nie finalnym systemem edycji leveli.
- Projekt docelowo uruchamiamy z `out/build/x64-Release`.
- Zapis powinien trafiac do repo `assets/...`, zeby zmiany byly latwe do commitowania.
