# Przewodnik po DevLevel

`DevLevel` jest roboczym levelem do szybkiego ustawiania propow i swiatla bez przebudowywania danych recznie.

## Co mozna zrobic

1. Kliknac prawym przyciskiem na mape, wpisac nazwe obiektu i zapisac jego pozycje.
2. Przesuwac glowne swiatlo klawiszami.
3. Zapisac aktualny zestaw swiatel do pliku JSON.

## Sterowanie

### Dodawanie propa

1. Kliknij PPM na mapie.
2. Wpisz nazwe propa.
3. Nacisnij `Enter`, aby zapisac.
4. Nacisnij `Esc`, aby anulowac.

Zapis trafia do:

```text
assets/data/static_objects_dev.json
```

Kazdy wpis ma postac:

```json
{
    "name": "tree",
    "x": 12.5,
    "y": -4.0
}
```

### Ustawianie swiatla

- `Arrow Up/Down` przesuwa swiatlo po osi Z
- `Arrow Left/Right` przesuwa swiatlo po osi X
- `Page Up/Page Down` przesuwa swiatlo po osi Y
- `S` zapisuje konfiguracje swiatla

Zapis trafia do:

```text
assets/maps/forest_lighting.json
```

## Co dokladnie zapisuje sie do pliku swiatla

Plik zawiera:
- kolor ambient
- liste swiatel
- typ, wlaczenie, pozycje, target i kolor kazdego swiatla

Przyklad:

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

## Jak wyglada typowy workflow

1. Uruchom `DevLevel`.
2. Ustaw swiatlo tak, zeby scena byla czytelna.
3. Nacisnij `S`.
4. Rozstaw propy przez PPM i `Enter`.
5. Przepisz lub wykorzystaj zapisane dane tam, gdzie maja trafic docelowo.

## Uwagi praktyczne

1. `DevLevel` edytuje dane robocze. To dobre miejsce do szybkiego strojenia, nie do finalnego gameplay scriptingu.
2. Pozycje zapisywane sa w przestrzeni XZ mapy.
3. Jesli uruchamiasz projekt ze standardowego katalogu builda, zapis preferuje pliki z repo `assets/...`, a nie ich kopie runtime. Dzieki temu zmiany sa latwiejsze do commitowania.
