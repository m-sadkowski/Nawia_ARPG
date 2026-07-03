# Agent Command Interface

## Cel

`AgentCommandInterface` jest warstwa wykonawcza dla przyszlych agentow. Nie
wybiera akcji, nie planuje, nie liczy threat table i nie waliduje strategii.
Przyjmuje jawna komende od zewnetrznego systemu decyzyjnego i probuje wykonac
ja na encji w istniejacej grze.

W kontekscie pracy inzynierskiej to fundament techniczny: przyszly system
wieloagentowy moze uzywac percepcji do obserwacji, a command interface do
wykonania decyzji.

Aktualny przeplyw:

```text
Future agent logic -> AgentCommandInterface -> Entity/Ability/Interactable/Map
```

## Glowny kod

- `src/core/game/agent/AgentCommandInterface.h`
- `src/core/game/agent/AgentCommandInterface.cpp`
- `src/core/Engine.h`
- `src/core/Engine.cpp`
- `docs/ET systems/Entity_Identity_and_Damage_Context.md`

## Obslugiwane komendy

- `MoveTo(position)` - prowadzi encje do pozycji, uzywajac sciezki z mapy, gdy
  mapa jest dostepna.
- `MoveToEntity(entity_id, desired_range)` - prowadzi encje do innej encji i
  konczy komende po wejsciu w zadany dystans.
- `Stop()` - zatrzymuje encje.
- `Attack(entity_id)` - ustawia target i utrzymuje podstawowy atak z ability
  slotu `0`.
- `CastAbility(slot, target_id)` - probuje uzyc ability typu `UNIT`.
- `CastAbility(slot, position)` - probuje uzyc ability typu `POINT`.
- `Interact(entity_id)` - podchodzi do obiektu interaktywnego i wykonuje
  `onInteract()` oraz `onInteractionCompleted()`.

Komendy sa przypisane do `EntityId` encji. W danym momencie aktywna jest jedna
komenda na agenta; nowa komenda zastapi poprzednia, a poprzednia zostanie
oznaczona jako `Cancelled / CommandReplaced`.

## Statusy

`AgentCommandStatus`:

- `Queued` - komenda przyjeta, ale jeszcze nie przetworzona w update.
- `Running` - komenda jest wykonywana.
- `Succeeded` - komenda zakonczyla sie powodzeniem.
- `Failed` - komenda nie moze zostac wykonana.
- `Cancelled` - komenda zostala anulowana albo zastapiona.

`AgentCommandFailureReason` opisuje najwazniejszy powod niepowodzenia, np.:

- `NoAgent`,
- `AgentUnavailable`,
- `NoTarget`,
- `TargetUnavailable`,
- `TargetOutOfRange`,
- `NoAbility`,
- `AbilityOnCooldown`,
- `AbilityTargetMismatch`,
- `ControlLocked`,
- `MovementRooted`,
- `InteractionUnavailable`,
- `PathUnavailable`,
- `CommandReplaced`,
- `Cancelled`.

## Najwazniejsze funkcje

### `AgentCommandInterface::submit(...)`

Przyjmuje ogolny `AgentCommandRequest`, nadaje `command_id` i zapisuje komende
jako aktywna dla danego agenta.

### `submitMoveTo(...)`

Tworzy komende ruchu do pozycji swiata. System buduje sciezke z `Map::findPath`
i przesuwa encje po waypointach.

### `submitMoveToEntity(...)`

Tworzy komende ruchu do encji docelowej. System przebudowuje sciezke do
aktualnej pozycji celu i konczy komende, gdy agent jest w `desired_range`.
To jest przydatne dla follow, stackowania, podejscia do ally albo ustawiania
sie przy celu bez wykonywania interakcji.

### `submitStop(...)`

Zatrzymuje encje. Dla `Player` uzywa `Player::stop()`, a dla pozostalych encji
ustawia cel ruchu na aktualna pozycje.

### `submitAttack(...)`

Ustawia target i utrzymuje podstawowy atak ze slotu `0`. Gdy target jest poza
preferowanym dystansem ataku, system probuje podejsc po sciezce. Preferowany
dystans jest domyslnie rowny okolo `2/3` `cast_range`, zeby agent nie
zatrzymywal sie idealnie na krawedzi zasiegu i mial zapas przeciw uciekajacemu
celowi. Sam `cast_range` nadal pozostaje prawdziwa walidacja ability. Komenda
konczy sie sukcesem, gdy target przestaje byc aktywny przez smierc lub
sekwencje umierania.

### `submitCastAbilityAtTarget(...)`

Wykonuje ability na encji docelowej. Waliduje slot, cooldown, typ targetowania
i zasieg.

### `submitCastAbilityAtPosition(...)`

Wykonuje ability w pozycji swiata. Jest przeznaczone dla ability typu `POINT`.

### `submitInteract(...)`

Podchodzi do obiektu implementujacego `Interactable` i wykonuje interakcje,
gdy agent jest w zasiegu.

`Interact(entity_id)` jest komenda wysokiego poziomu: nie trzeba wykonywac
osobno `MoveToEntity` i dopiero potem `Interact`.

### `update(...)`

Jest wolane przez `Engine` po update poziomu i przed update encji. Dzieki temu
komenda moze ustawic ruch lub ability jeszcze przed fizycznym krokiem encji w
tej samej klatce.

## Granice systemu

Ten system nie decyduje:

- ktora komenda jest najlepsza,
- kiedy agent ma zmienic target,
- czy taktyka druzyny jest poprawna,
- czy role sa dobrze rozdzielone,
- czy zachowanie spelnia zalozenia raidowe.

To nalezy do przyszlego algorytmu pracy inzynierskiej. `AgentCommandInterface`
ma tylko bezpiecznie wykonac polecenie i zwrocic status.

## Uwagi integracyjne

Obecny `Friend` nadal ma hardcoded behavior, jesli nie ma podpietego
`AllyBrain`. To znaczy, ze przyszly brain powinien przejac sterowanie ally
konsekwentnie, aby nie mieszac decyzji hardcoded AI z komendami systemu ET.
