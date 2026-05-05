# Przewodnik po interakcjach

Obiekty interaktywne to encje, z ktorymi gracz moze wejsc w interakcje przez klikniecie albo wejscie w obszar triggera.

## Glowny podzial

- `Interactable` - interfejs kontraktu interakcji.
- `InteractiveClickable` - obiekt klikany myszka, np. skrzynia albo NPC.
- `InteractiveTrigger` - obszar aktywowany wejsciem, np. teleport albo checkpoint.

## `Interactable`

Kontrakt:

```cpp
virtual void onInteract(Entity& instigator) = 0;
virtual void onTriggerEnter(Entity& other) = 0;
virtual bool canInteract() const;
virtual float getInteractionRange() const;
```

`onInteract(...)` jest dla klikniecia, `onTriggerEnter(...)` dla wejscia w obszar.

## Klikalne obiekty

Dziedzicz po `InteractiveClickable`, gdy gracz ma kliknac konkretny model.

Przyklady:

- `Chest`,
- `Cat`,
- przyszli kupcy/NPC.

Klikanie jest oparte o raycast w mesh modelu i fallback bounding box. To oznacza, ze model z dziwna geometria moze byc trudniejszy do trafienia kursorem. W takim przypadku popraw model, bounding box albo interaction range.

## Triggery

Dziedzicz po `InteractiveTrigger`, gdy efekt ma odpalic sie po wejsciu w obszar.

Przyklady:

- `Teleport`,
- `Checkpoint`.

Trigger powinien miec collider, zwykle `RectangleCollider`, bo jego zadaniem jest wykrycie wejscia encji w obszar.

## Zasieg interakcji

`PlayerController` moze zapamietac klikniety obiekt i podejsc do niego. Interakcja odpali sie dopiero, gdy dystans do obiektu bedzie mniejszy od `getInteractionRange()`.

Skrzynie i NPC moga miec wiekszy zasieg niz zwykly obiekt, jezeli stoja na nierownym terenie albo maja duzy model.

## Skrzynie

`Chest`:

- moze losowac loot z `Loottable`,
- moze byc zablokowana kluczem,
- udostepnia `Backpack` dla `ChestUI`,
- otwiera UI przez `UIHandler::openContainer(...)`.

Zawartosc skrzyni powinna pochodzic z JSON albo loottable, a nie z UI.

## NPC

Aktualny NPC to `Cat`.

`Cat`:

- moze miec loot table,
- reaguje na oddanie ryby,
- po zakonczeniu questa nie powinien ponownie otwierac inventory z nagroda,
- moze delegowac dialog do `DialogueManager`.

Docelowo nowe NPC powinny dostawac dialog/quest binding z danych, ale obecny system ma jeszcze czesc logiki w C++.

## Dobre praktyki

- Klikalne obiekty nie powinny udawac triggerow.
- Triggery powinny miec czytelny collider.
- UI otwieraj przez `UIHandler`, nie bezposrednio z encji.
- Interakcje z itemami trzymaj w `Item`/`Inventory`, nie w klasach UI.
- Dla problemow z hoverem najpierw sprawdz mesh i bounding box.
