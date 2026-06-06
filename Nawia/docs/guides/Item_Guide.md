# Przewodnik po itemach

Ten dokument opisuje baze przedmiotow, ekwipunek, wyposazenie i tabele lootu.

## Glowny model

Najwazniejsze typy:

- `Item` - bazowy przedmiot z ID, nazwa, opisem, slotem, ikona i statystykami.
- `EquipmentSlot` - slot, do ktorego przedmiot moze zostac zalozony.
- `Backpack` - lista itemow o stalej pojemnosci.
- `Equipment` - mapa slotow zalozonego wyposazenia.
- `ItemDatabase` - wczytuje template'y z JSON i tworzy kopie przez `clone()`.
- `Loottable` - wczytuje tabele lootu i tworzy wpisy z kopii itemow.

## Ownership

- Tekstury itemow sa `std::shared_ptr<Texture2D>`, bo pochodza z cache `ResourceManager`.
- `ItemDatabase` trzyma template'y itemow.
- `createItem(id)` zwraca kopie template'u, nie oryginal.
- `Backpack` i `Equipment` trzymaja `std::shared_ptr<Item>`, bo item moze byc przenoszony miedzy inventory, chest i UI.

## `items.json`

Przyklad:

```json
{
    "id": 7,
    "name": "Koci Miecz",
    "description": "Nagroda od kota.",
    "slot": "Weapon",
    "texture": "assets/textures/items/cat_sword.png",
    "stats": {
        "damage": 15
    }
}
```

Obslugiwane sloty:

- `None`,
- `Head`,
- `Neck`,
- `Chest`,
- `Legs`,
- `Feet`,
- `Weapon`,
- `OffHand`,
- `Ring`.

## Typy itemow

Subclassy itemow istnieja tylko tam, gdzie przedmiot ma wyspecjalizowane statystyki albo zachowanie:

- `Weapon`,
- `Offhand`,
- `Head`,
- `Necklace`,
- `Chestplate`,
- `Legs`,
- `Boots`,
- `Ring`.

Kazdy subclass powinien nadpisywac `clone()`.

## Itemy odblokowujace umiejetnosci

`Ksiega Baby Jagi` ma ID `18` i jest obslugiwana jako fabularny consumable przy
zabraniu z kontenera. UI nie przenosi jej do plecaka: wywoluje
`Player::unlockFireballAbility()`, usuwa item ze skrzyni i zapisuje permanentny
unlock w profilu gracza jako `fireball_unlocked`. Fireball trafia do slotu
ability `2`, zeby nie nadpisywac podstawowego ataku ani drugiego ciosu.

`Cichobiegi` maja ID `19`. To prezentacyjny item typu `Feet` z bardzo duzym
bonusem `movement_speed`; `FirstLevel` zaklada go graczowi automatycznie przy
nowej grze na Wczorze.

## Backpack

`Backpack`:

- ma stala pojemnosc,
- `addItem(...)` dodaje do pierwszego wolnego slotu,
- nie przyjmuje `nullptr`,
- `removeItem(index)` czysci slot,
- `getItem(index)` zwraca item albo `nullptr`.

UI nie powinno decydowac o semantyce itemu. UI tylko pyta backpack/equipment o dane i wysyla klikniecia.

## Equipment

`Equipment::equip(...)`:

- odrzuca pusty item,
- zwraca poprzedni item ze slotu,
- dla `EquipmentSlot::None` oddaje item bez zakladania.

To pozwala latwo zamieniac itemy miedzy plecakiem i slotem.

## Loottable

`assets/data/loottables.json` mapuje typ tabeli na ID itemow i szanse.

Przyklad:

```json
{
    "type": "CHEST_NOOB",
    "loot": {
        "3": 1.0,
        "7": 0.25
    }
}
```

`Loottable` loguje:

- niepoprawne ID,
- niepoprawna szanse,
- odwolanie do nieznanego itemu.

## Dodanie nowego itemu

1. Dodaj teksture do `assets/textures/items/`.
2. Dodaj wpis w `assets/data/items.json`.
3. Uzyj istniejacego slotu albo dodaj nowy w `EquipmentSlot`.
4. Jezeli potrzebny jest nowy typ statystyk, dodaj subclass i obsluge w `ItemDatabase`.
5. Dodaj item do loottable albo level data.
6. Jezeli item ma znikac przy zabraniu i zmieniac stan gracza, obsluz go w przeplywie transferu kontenera.

## Dobre praktyki

- Sciezki tekstur zapisuj wzgledem katalogu runtime, np. `assets/textures/items/key.png`.
- Nie trzymaj logiki questow w UI itemow.
- Nie zwracaj template'u z `ItemDatabase`, jezeli caller ma modyfikowac item.
- Nowe statystyki czytaj przez helper z fallbackiem, zeby brak pola w JSON nie crashowal gry.
