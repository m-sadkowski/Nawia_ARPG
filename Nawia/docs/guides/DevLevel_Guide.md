# Kreator Poziomów (DevLevel)

`DevLevel` to zaawansowane narzędzie wewnątrzsilnikowe służące do szybkiego projektowania poziomów, ustawiania oświetlenia oraz definiowania punktów spawnu encji bezpośrednio w świecie gry.

## Uruchomienie
Edytor jest dostępny jako specjalna klasa poziomu. Pozwala na swobodne poruszanie się "kamerą gracza" z dużą prędkością w celu szybkiej inspekcji mapy.

### Sterowanie ogólne
- **WSAD**: Ruch postacią/kamerą.
- **Lewy SHIFT (przytrzymanie)**: Szybki przelot (tryb turbo).
- **Prawy Przycisk Myszy (PPM)**: Otwiera menu kontekstowe w miejscu wskazanym kursorem na ziemi.
- **DELETE / X**: Usuwa obiekt znajdujący się bezpośrednio pod kursorem myszy (celuj w kolorowy sześcian).
- **S**: Zapisuje aktualną konfigurację oświetlenia mapy.
- **ESC**: W menu edytora cofa lub zamyka okno; w grze otwiera menu pauzy.

## Dodawanie Obiektów
Kliknięcie **PPM** na ziemi otwiera menu wyboru typu obiektu. Możesz dodać:

### 1. Spawner (Przeciwnicy)
Pozwala na zdefiniowanie grupy przeciwników (np. Devil, Bandit).
- **Liczba sztuk**: Ile encji ma się pojawić.
- **Spawn Radius**: Obszar (żółty okrąg), w którym losowo rozstawione zostaną encje.
- **Trigger Radius**: Odległość (pomarańczowy okrąg), po przekroczeniu której przeciwnicy zostaną "obudzeni".

### 2. Skrzynia (Loot)
- Możliwość nadania nazwy.
- **Loot System**: Pozwala wybrać przedmioty z bazy danych gry (`ItemDatabase`), które znajdą się wewnątrz skrzyni.

### 3. NPC / Rozmowy
- Pozwala postawić postać niezależną (np. Kota) z przypisaną klasą logiki rozmowy.

### 4. Prop (Obiekt Statyczny)
- Pozwala postawić dekoracje (drzewa, kamienie itp.) z podaniem ścieżki do tekstury/modelu.

### 5. Teleport
- Definiuje punkt przejścia do innej mapy.

## Wizualizacja i Debug
W trybie edytora obiekty są reprezentowane przez kolorowe sześciany ułatwiające orientację:
- **Czerwony**: Spawner.
- **Złoty**: Skrzynia.
- **Błękitny**: NPC.
- **Zielony**: Prop.
- **Fioletowy**: Teleport.

Dodatkowo spawnerzy posiadają trójwymiarowe "ściany" zasięgu:
- **Żółta ścianka**: Zasięg spawnu.
- **Pomarańczowa ścianka**: Zasięg aktywacji (Trigger).

## Testowanie Poziomu
W menu głównym edytora (PPM) znajduje się przycisk **TESTUJ POZIOM**.
- **Działanie**: Natychmiastowo czyści świat i spawnuje wszystkie postawione obiekty jako prawdziwe encje gry.
- **Logika**: Przeciwnicy będą uśpieni, dopóki nie wejdziesz w ich `Trigger Radius`, co pozwala sprawdzić balans poziomu bez restartu gry.

## Zapis i Struktura Danych
System automatycznie wczytuje i zapisuje sesje w katalogu:
`assets/data/dev/`

Pliki są podzielone na kategorie:
- `new_level_spawners.json`
- `new_level_chests.json`
- `new_level_npcs.json`
- `new_level_props.json`
- `new_level_teleports.json`

Po zakończeniu prac w edytorze, dane z tych plików można skopiować do docelowych plików definicji poziomu.

## Sterowanie Oświetleniem
Możesz w czasie rzeczywistym przesuwać główne światło mapy:
- **Strzałki (Góra/Dół/Lewo/Prawo)**: Przesunięcie w poziomie (X/Z).
- **Page Up / Page Down**: Przesunięcie w pionie (Y).
- **S**: Zapis do `assets/maps/forest_lighting.json`.
