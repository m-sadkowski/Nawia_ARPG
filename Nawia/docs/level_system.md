# System Leveli, Lokacji i Encji — Dokumentacja

## Spis treści
1. [Architektura ogólna](#architektura-ogólna)
2. [Tworzenie nowego levelu](#tworzenie-nowego-levelu)
3. [Plik JSON level_entities](#plik-json-level_entities)
4. [Typy encji w JSON](#typy-encji-w-json)
5. [System aktywacji encji (dormant / proximity)](#system-aktywacji-encji-dormant--proximity)
6. [Jak to działa w runtime](#jak-to-działa-w-runtime)
7. [Stan aktualny — NPC, Questy, Dialogi](#stan-aktualny--npc-questy-dialogi)
8. [Plan na przyszłość](#plan-na-przyszłość)

---

## Architektura ogólna

```
Engine
 └── LevelManager
      └── Level (baza abstrakcyjna)
           ├── _map (Map — geometria 3D)
           ├── _spawn_manager (SpawnManager — pre-tworzy i aktywuje encje z JSON)
           └── getLocations() → lista nazw lokacji w obrębie levelu

assets/data/
 ├── level_entities/      ← pliki JSON z definicjami encji per level
 │   ├── demo_level.json
 │   ├── mroczny_las.json
 │   ├── starozytne_lochy.json
 │   └── pobojowisko.json
 ├── items.json            ← baza przedmiotów
 ├── loottables.json       ← tabele łupów
 └── abilities.json        ← statystyki umiejętności
```

**Przepływ danych:**
```
Level::onEnter()
  → loadMap()                 // ładuje geometrię 3D
  → loadSpawns(engine)        // ładuje JSON → SpawnManager
    → SpawnManager::loadFromJson(path, engine, map, initial_location)
      → EntityFactory::create() dla KAŻDEJ encji ze WSZYSTKICH lokacji
      → encje w startowej lokacji z trigger_radius == 0 → od razu aktywne
      → encje w startowej lokacji z trigger_radius > 0 → dormant (proximity)
      → encje w INNYCH lokacjach → dormant (aktywowane przy zmianie lokacji)
      → wszystkie dodane do EntityManager
    → ustawia pozycję gracza z "player_spawn"

Level::update() [co klatkę — LEKKIE, zero alokacji]
  → SpawnManager::update(player_pos, current_location)
    → filtruje po current_location (ignoruje inne lokacje)
    → dla każdego nieaktywnego SpawnPoint:
      → sprawdź odległość gracza
      → jeśli w zasięgu → entity->setDormant(false)  ← tylko flip flagi!
```

---

## Tworzenie nowego levelu

### Krok 1: Klasa C++

Utwórz plik nagłówkowy, np. `src/world/level/NowyLevel.h`:

```cpp
#pragma once
#include "Level.h"

namespace Nawia::World {

class NowyLevel : public Level {
public:
    void onEnter(Core::Engine* engine) override;

    [[nodiscard]] std::string getName() const override { return "Nowy Level"; }
    
    [[nodiscard]] std::string getSpawnFilePath() const override {
        return "../assets/data/level_entities/nowy_level.json";
    }
    
    [[nodiscard]] std::vector<std::string> getLocations() const override {
        return {"Lokacja A", "Lokacja B", "Lokacja C"};
    }
};

} // namespace Nawia::World
```

Utwórz implementację `src/world/level/NowyLevel.cpp`:

```cpp
#include "NowyLevel.h"
#include <Map.h>
#include <Engine.h>
#include <Logger.h>

namespace Nawia::World {

void NowyLevel::onEnter(Core::Engine* engine) {
    Core::Logger::debugLog("Ladowanie: Nowy Level...");

    _map = std::make_unique<Core::Map>(engine->getResourceManager());
    _map->loadMap("sciezka/do/mapy.glb", 2.0f);

    engine->getEntityManager().clearNonPlayerEntities();
    loadSpawns(engine);  // ← to robi wszystko z JSONa
}

} // namespace Nawia::World
```

### Krok 2: Rejestracja w Engine

W `Engine.cpp`, w konstruktorze:
```cpp
_level_manager->registerLevel(std::make_shared<World::NowyLevel>());
```

### Krok 3: Plik JSON

Utwórz `assets/data/level_entities/nowy_level.json` (patrz sekcja poniżej).

---

## Plik JSON level_entities

Każdy level ma swój plik JSON. Struktura:

```json
{
    "player_spawn": {
        "Lokacja A": { "x": 5.0, "y": 10.0 },
        "Lokacja B": { "x": 0.0, "y": 0.0 }
    },
    "entities": [
        {
            "location": "Lokacja A",
            "type": "devil",
            "name": "Mocny Devil",
            "x": 10.0, "y": 15.0,
            "hp": 200,
            "trigger_radius": 12.0,
            "spawn_radius": 3.0
        }
    ]
}
```

### Klucze główne

| Klucz | Opis |
|---|---|
| `player_spawn` | Obiekt: pozycja gracza per lokacja. Klucz = nazwa lokacji z `getLocations()`. |
| `entities` | Tablica obiektów — encje do stworzenia (pre-loaded przy wejściu na mapę). |

### Wspólne pola encji

| Pole | Typ | Domyślnie | Opis |
|---|---|---|---|
| `location` | string | (wymagane) | Nazwa lokacji, do której należy encja |
| `type` | string | (wymagane) | Typ encji: `devil`, `bandit`, `walking_dead`, `chest`, `npc`, `static_object`, `checkpoint` |
| `name` | string | zależy od typu | Wyświetlana nazwa |
| `x`, `y` | float | 0.0 | Pozycja w świecie (XZ) |
| `trigger_radius` | float | 0.0 | 0 = aktywna od razu; >0 = dormant, aktywowana gdy gracz podejdzie na tę odległość |
| `spawn_radius` | float | 0.0 | Losowy offset od pozycji (tworzy naturalną wariację) |
| `respawnable` | bool | false | Czy encja może się odrodzić po śmierci |
| `respawn_cooldown` | float | 0.0 | Czas (sekundy) do odrodzenia |

---

## Typy encji w JSON

### Wrogowie: `devil`, `bandit`, `walking_dead`

```json
{
    "type": "devil",
    "name": "Demon Strażnik",
    "x": 10.0, "y": 15.0,
    "hp": 200,
    "trigger_radius": 15.0
}
```

| Pole | Typ | Opis |
|---|---|---|
| `hp` | int | Punkty życia |
| `abilities` | string[] | (tylko bandit) Lista umiejętności, np. `["KnifeThrow"]` |

Wrogowie dostają automatycznie gracza jako cel (`setTarget`).

### Skrzynia: `chest`

```json
{
    "type": "chest",
    "name": "Stara Skrzynia",
    "x": -13.44, "y": -21.44,
    "loottable": "CHEST_NOOB",
    "trigger_radius": 0
}
```

| Pole | Typ | Opis |
|---|---|---|
| `loottable` | string | Nazwa tabeli łupów: `CHEST_NOOB`, `CHEST_BAD`, `CAT` |
| `items` | int[] | Opcjonalne — konkretne ID itemów do dodania |
| `locked` | bool | Czy skrzynia jest zablokowana |
| `key_id` | int | ID klucza wymaganego do otwarcia |

### NPC: `npc`

```json
{
    "type": "npc",
    "npc_class": "cat",
    "name": "Kot Olga",
    "x": 0.35, "y": -18.23,
    "loottable": "CAT",
    "trigger_radius": 0
}
```

| Pole | Typ | Opis |
|---|---|---|
| `npc_class` | string | Klasa NPC: aktualnie tylko `"cat"` |
| `loottable` | string | Tabela łupów (opcjonalne) |

**⚠️ UWAGA:** Dialogi i questy NPC są aktualnie **hardkodowane w C++** (patrz sekcja poniżej).

### Obiekt statyczny: `static_object`

```json
{
    "type": "static_object",
    "name": "Drzewo",
    "x": 5.0, "y": 5.0,
    "hp": 9999,
    "texture": "assets/textures/chest.png",
    "trigger_radius": 0
}
```

### Checkpoint: `checkpoint`

```json
{
    "type": "checkpoint",
    "name": "Punkt Kontrolny",
    "x": 20.0, "y": 20.0,
    "trigger_radius": 0
}
```

---

## System aktywacji encji (dormant / proximity)

**Wszystkie encje ze wszystkich lokacji** są tworzone przy wejściu na mapę (zero lagów w runtime).

Encja startuje jako **dormant** jeśli:
1. **Jest w innej lokacji** niż startowa (zawsze dormant, czeka na zmianę lokacji)
2. **Ma `trigger_radius > 0`** w startowej lokacji (proximity — budzi się na zbliżenie)

Encja jest od razu **aktywna** jeśli:
- Jest w **startowej lokacji** gracza **I** ma `trigger_radius == 0`

### Natychmiastowe (`trigger_radius: 0`, aktualna lokacja)
- Skrzynki, NPC, checkpointy, obiekty statyczne
- Tworzone i **od razu aktywne** przy wejściu do lokacji

### Zbliżeniowe (`trigger_radius > 0`, aktualna lokacja)
- Wrogowie
- Tworzone przy wejściu, ale **dormant** (niewidoczne, bez update/render/kolizji)
- Aktywowane gdy gracz podejdzie na odległość `trigger_radius`
- Pozwala na elementy zaskoczenia (zasadzki) bez lagów

### Inna lokacja (dowolny `trigger_radius`)
- Wszystkie encje dormant
- Aktywowane dopiero gdy gracz zmieni lokację (teleport, NYI)
- `SpawnManager::update()` filtruje po `current_location`

### Co oznacza "dormant"?
Encja dormant:
- ❌ Nie jest renderowana
- ❌ Nie jest aktualizowana (zamrożona pozycja, brak AI)
- ❌ Nie uczestniczy w kolizjach
- ❌ Nie reaguje na kliknięcia/hover
- ✅ Istnieje w pamięci (model załadowany, gotowa do aktywacji)

### Losowa pozycja (`spawn_radius > 0`)
- Pozycja losowana przy tworzeniu (w `loadFromJson`), nie przy aktywacji
- Idealne dla grup wrogów (ten sam punkt, różne pozycje)

### Respawn
- `respawnable: true` + `respawn_cooldown: 30.0` → wróg odradza się po 30 sekundach
- Aktualnie nie używane, ale gotowe do użycia

---

## Jak to działa w runtime

```
1. Gracz wybiera level w menu → Engine::handleInput()
2. LevelManager::loadLevel("Nazwa") → Level::onEnter()
3. Level ładuje startową mapę 3D i woła loadSpawns()
4. SpawnManager::loadFromJson(path, engine, map, initial_location):
   → parsuje JSON ze WSZYSTKIMI lokacjami
   → EntityFactory::create() dla KAŻDEJ encji (modele, tekstury, animacje)
   → Inne lokacje / trigger_radius > 0 → entity->setDormant(true)
   → Startowa lokacja i trigger_radius == 0 → od razu aktywne
   → wszystkie dodane do EntityManager
5. Pozycja gracza ustawiana z "player_spawn" na startowej lokacji
6. Co klatkę: Level::update() → SpawnManager::update()
   → LEKKIE: tylko sprawdzenie odległości dla aktualnej lokacji
   → jeśli gracz w zasięgu → entity->setDormant(false) ← flip flagi
7. Przy wejściu w Teleport: Level::changeLocation()
   → Level ładuje nową geometrię (jeśli nadpisano)
   → SpawnManager zamraża starą lokację i budzi nową
   → Gracz ląduje na nowym player_spawn
8. Przy wyjściu: Level::onExit()
   → clearNonPlayerEntities()
   → SpawnManager::reset()
```

---

## Stan aktualny — NPC, Questy, Dialogi

### ⚠️ Częściowo przeniesione do zewnątrz / w trakcie zmian:

1. **Dialogi** — `DialogueManager::createCatDialogue()` wciąż buduje `DialogueTree` 
   z C++ lambdami (np. `openContainer`, `closeDialogue`). Wymagają ustrukturyzowania w `dialogues.json`.

2. **System Questów (Wielki Krok Naprzód)** — od strony strukturalnej questy są w `quests.json` i wykorzystują Data-Driven Design! Eventy rozwijające je (jak zagadanie, odebranie przedmiotu, wejście w strefę) spłaszczono wewnątrz powiadomień `QuestManager::notify...()`. (W C++ hardkodowane są jeszcze konkretne nagrody dla niektórych NPC — m.in `Cat::onInteract()` wciąż dodatkowo bezpośrednio manipuluje ekwipunkiem NPC oprócz uruchamiania powiadomień. Tę małą część również będzie trzeba delegować w całości na `quests.json`).

3. **Klasy NPC** — jedyna unikalna klasa to `Cat`. Nowe NPC wciąż wymagają nowych klas C++.

### Co jest w JSON:

Z JSONa ładowane jest **tylko położenie i podstawowa konfiguracja** NPC:
- pozycja (`x`, `y`)
- nazwa (`name`)
- klasa NPC (`npc_class`)
- tabela łupów (`loottable`)
- trigger radius

---

## Plan na przyszłość

### 1. Dialogi w JSON (`dialogues.json`)

```json
{
    "dialogues": {
        "cat_greeting": {
            "nodes": [
                {
                    "id": 0,
                    "speaker": "Kot Olga",
                    "text": "Miau. Pomożesz mi?",
                    "options": [
                        { "text": "Jasne!", "action": "open_inventory" },
                        { "text": "Nie.", "action": "close" }
                    ]
                }
            ]
        }
    }
}
```

- `action` jako string-enum zamiast lambdy → C++ mapuje stringi na akcje w runtime
- Akcje do obsługi: `open_inventory`, `close`, `next_node`, `start_quest`, `give_item`
- `DialogueManager::loadDialogues()` ładuje raz przy starcie
- `DialogueManager::getDialogue(id, engine, npc)` zwraca gotowy `DialogueTree`

### 2. Pełne wdrożenie nagród z Questów w `quests.json` i generyczny odbiór

Obecnie eventy `notifyNPCTalked()`, `notifyItemDelivered()`, `notifyKil()` itp. prężnie zasilają system zadań. Chociaż JSON definiuje już systematycznie nagrody jak XP czy przedmioty (np. `rewards: { exp: 50 }`), należy upewnić się, że `Cat::onInteract()` i podobne instrukcje z klas fizycznych oddadzą odpowiedzialność wydawania Katan i zamykania wątków pod same nagrody zdefiniowane w Questach i `DialogueManager`.

### 3. NPC z quest/dialogue binding w level_entities

```json
{
    "type": "npc",
    "npc_class": "cat",
    "name": "Kot Olga",
    "x": 0.35, "y": -18.23,
    "quest_id": "cat_fish_quest",
    "dialogue_id": "cat_greeting",
    "loottable": "CAT"
}
```

### 4. Bossy i Raidy

Nowe typy encji w `EntityFactory`:
- `"boss"` — boss z unikalnymi mechanikami (fazy walki, specjalne ataki)
- Boss fight rooms — osobne lokacje w obrębie levelu
- Raid system — grupowe walki z bossem (jeśli dotyczy)

### 5. Generyczna klasa NPC

Zamiast jednej klasy per NPC (`Cat`, `Kupiec`, `Straznik`), stworzyć
generyczną klasę `NPC` z konfiguracją z JSON:
- model 3D
- animacje
- dialogi
- questy
- inventory/loottable
- interactionRange

---

## Pliki źródłowe

| Plik | Rola |
|---|---|
| `src/entity/Entity.h/.cpp` | Klasa bazowa — flaga `_dormant`, guard w `update()` i `render()` |
| `src/Core/game/EntityManager.cpp` | Skip dormant encji w: render, kolizjach, hover, click |
| `src/world/level/Level.h` | Baza abstrakcyjna — SpawnManager, loadSpawns(), update() |
| `src/world/level/Level.cpp` | Domyślne implementacje: update(), onExit(), loadSpawns() |
| `src/world/spawn/SpawnManager.h/.cpp` | Pre-tworzy encje z JSON, aktywuje na proximity |
| `src/world/spawn/SpawnPoint.h` | Struct z danymi jednego spawn pointa + referencja do encji |
| `src/world/spawn/EntityFactory.h/.cpp` | Tworzy encje z JSON po typie |
| `src/world/level/LevelManager.h/.cpp` | Rejestr leveli, przełączanie |
| `src/Core/Engine.cpp` | Rejestracja leveli, główna pętla |
| `src/Core/game/DialogueManager.h/.cpp` | Tworzenie dialogów NPC (hardkodowane, do przeniesienia do JSON) |
