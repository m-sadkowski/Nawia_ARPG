# Przewodnik po UI

Ten dokument opisuje aktualny uklad UI, odpowiedzialnosci klas i zasady dodawania nowych paneli.

## Glowny podzial

- `UIHandler` - koordynuje HUD, menu i panele.
- `InventoryUI` - rysuje ekwipunek gracza i sloty equipment.
- `ChestUI` - rysuje zawartosc otwartej skrzyni.
- `StatsUI` - rysuje statystyki gracza.
- `QuestUI` - rysuje questbook.
- `DialogueUI` - rysuje dialog i opcje odpowiedzi.
- `SettingsMenu` i `LevelSelectMenu` - osobne ekrany menu.

`UIHandler` posiada komponenty UI przez `std::unique_ptr`. Managerow gameplayowych uzywa jako nieposiadajacych wskaznikow.

## Skala UI

Globalna skala przechodzi przez `Core::GlobalScaling`.

Zasady:

- stale layoutu trzymaj w bazowej rozdzielczosci,
- przed rysowaniem przepuszczaj wymiary przez `GlobalScaling::scaled(...)`,
- panele musza miec limit, zeby nie wychodzily poza ekran,
- domyslna skala moze byc lekko zmniejszona mnoznikiem bazowym, ale slider uzytkownika nadal powinien dzialac.

## Inventory, chest i stats

Aktualny layout:

- inventory jest glownym panelem po lewej,
- stats ma byc wyrownany do lewej z inventory albo ustawiany tak, by nie nachodzil na inne panele,
- chest jest obok inventory i nie powinien wymuszac otwierania questbooka,
- questbook nie otwiera sie, gdy chest UI jest aktywne.

UI nie powinno bezposrednio modyfikowac danych poza wywolaniem metod inventory/equipment/container.

## Ability bar i orby

HUD ability:

- ikony ability rysowane sa pod ramka baru,
- rama ability bara jest osobna tekstura,
- orby HP/level maja wlasne ramki,
- cooldown i tekst powinny byc czytelne przy zmianie skali.

Po zmianie assetow sprawdz przerwy miedzy ikonami i ramka, bo tekstura ability bara ma wlasny padding.

## Przyciski

Wspolny wyglad przyciskow jest w `UIHandler::drawMenuButton(...)`.

Nowe menu powinny uzywac tego helpera, zeby:

- tekst mial te sama animacje hover,
- hover nie robil zoltych prostokatow poza tekstura,
- rozmycie i filtrowanie tekstur byly spojne.

## Questbook

`QuestUI` ma dwie kolumny:

- lista questow,
- szczegoly wybranego questa.

Tekst musi zawijac sie w jasnym obszarze ksiazki, nie po ramie. Zakladki sa elementem layoutu questbooka i powinny trzymac sie lewej strony, gdy jedna lista jest pusta.

## Dialogue

`DialogueUI` dobiera wysokosc okna dynamicznie do tekstu i liczby opcji.

Zasady:

- tekst nie moze wychodzic poza panel,
- opcje musza miec stabilne prostokaty klikniecia,
- input gameplayowy jest blokowany, gdy dialog jest otwarty.

## Dodanie nowego panelu

1. Utworz klase panelu w `src/ui/...`.
2. Daj jej `loadResources(...)`, jezeli uzywa tekstur.
3. Daj `render(...)` i `handleInput(...)`.
4. Wlasnosc panelu trzymaj w `UIHandler` przez `std::unique_ptr`.
5. Zadbaj o `isMouseOverUI()` i `isInputBlocked()`, jezeli panel blokuje gameplay.

## Dobre praktyki

- Nie mieszaj logiki gameplayowej z rysowaniem.
- UI ma pytac model o dane, a nie przechowywac prawde o itemach/questach.
- Nowe tekstury UI laduj przez `ResourceManager`.
- Wszystkie teksty i komentarze w UI pisz po polsku.
- Po zmianie layoutu sprawdz minimum: inventory, chest, stats, questbook, dialogue, pause menu i ability bar.
