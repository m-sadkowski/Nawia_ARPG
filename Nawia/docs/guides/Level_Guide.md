# Przewodnik po levelach, mapach i spawnach

Ten dokument opisuje aktualny sposob dodawania levelu. Nowe poziomy korzystaja
z plikow lokacji tworzonych przez Kreator leveli w `assets/data/locations`.

## Glowny przeplyw

Level robi zwykle:

1. Wskazuje liste plikow lokacji.
2. Wywoluje `loadLocations(engine, ..., "Lokacja startowa")`.
3. Loader laduje definicje map, modele, navmesh, spawn gracza i obiekty.
4. Encje ze wszystkich lokacji sa tworzone przy starcie levelu, ale encje z
   nieaktywnych lokacji startuja jako dormant.

## Dodanie nowego levelu

Header:

```cpp
#pragma once

#include <Level.h>

namespace Nawia::World {

class MojaDolinaLevel : public Level {
public:
	void onEnter(Core::Engine* engine) override;

	[[nodiscard]] std::string getName() const override { return "Moja Dolina"; }

	[[nodiscard]] std::vector<std::string> getLocations() const override {
		return {"Polana", "Jaskinia"};
	}
};

} // namespace Nawia::World
```

Implementacja:

```cpp
#include "MojaDolinaLevel.h"

#include <Engine.h>
#include <Logger.h>

namespace Nawia::World {

namespace {
	const std::vector<LevelLocationFile> MOJA_DOLINA_LOCATIONS = {
		{"Polana", "assets/data/locations/polana.json"},
		{"Jaskinia", "assets/data/locations/jaskinia.json"},
	};
}

void MojaDolinaLevel::onEnter(Core::Engine* engine)
{
	Core::Logger::debugLog("Ladowanie poziomu Moja Dolina...");
	loadLocations(engine, MOJA_DOLINA_LOCATIONS, "Polana");
}

} // namespace Nawia::World
```

Na koncu zarejestruj level w `Engine.cpp`:

```cpp
_level_manager->registerLevel(std::make_shared<World::MojaDolinaLevel>());
```

## Pliki lokacji

Kreator zapisuje dwa pliki dla kazdej lokacji:

```text
assets/data/locations/polana.json
assets/data/locations/objects_polana.json
```

Plik lokacji zawiera mape, transformacje, navmesh i spawn gracza. Plik
`objects_*.json` zawiera encje: przeciwnikow, skrzynie, NPC, teleporty,
checkpointy, propsy i boss triggery.

## Teleporty

Teleporty uzywaja pola `target_location`, ktore musi byc identyczne z nazwa
lokacji podana w klasie levelu i w JSON lokacji. Przy przejsciu loader zmienia
mape, przenosi gracza na spawn docelowej lokacji i aktywuje encje tej lokacji.
Encja teleportu uzywa statycznego modelu `assets/models/teleport.glb`.

## Typy encji

Aktualnie w JSON sa obslugiwane m.in.:

- `bandit`,
- `devil`,
- `walking_dead`,
- `friend`,
- `chest`,
- `npc` z `npc_class`,
- `static_object`,
- `teleport`,
- `checkpoint`,
- `boss_trigger`.

Nowy typ dodaj w `EntityFactory`, a nie bezposrednio w levelu.
