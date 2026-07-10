# Nawia: Slavic Action RPG

![Language](https://img.shields.io/badge/Language-C%2B%2B20-blue)
![Library](https://img.shields.io/badge/Library-raylib-red)
![Build](https://img.shields.io/badge/Build-CMake-green)
![Status](https://img.shields.io/badge/Status-Beta-orange)

Nawia is an isometric action RPG built in C++ with raylib and a custom game
engine. The game is set in a dark fantasy world inspired by Slavic mythology,
folklore, and bestiaries. It focuses on dynamic hack'n'slash combat, atmospheric
storytelling, quests, NPC interactions, and multi-stage boss encounters.

The project combines a playable RPG prototype with technical systems prepared
for further research and experimentation. The current architecture is not a full
ECS. Runtime gameplay is centered around `Engine`, `EntityManager`, `Entity`,
actor and interactable subclasses, data-driven factories, and focused manager
systems.

## Academic Context

Nawia is developed at Gdansk University of Technology as a game project and a
foundation for Engineering Thesis work.

The Engineering Thesis part of the project focuses on believable fake
multiplayer in an action RPG. Planned and ongoing thesis work covers systems
such as:

- agents with limited perception,
- tactical communication between agents,
- role-based behavior, such as tank, healer, or damage-oriented roles,
- imperfect human-like decision-making, including delay, mistakes, and limited
  information,
- map pings and lightweight team coordination,
- fake chat and player-like feedback,
- grouping, retreat, support, and help-request behavior,
- boss mechanics and raid-style telemetry,
- monitoring tools for debugging combat, perception, and agent commands.

The game acts as the runtime environment for these systems, while the telemetry
layer and monitor provide data for analysis, testing, and thesis documentation.

## Main Modules

```text
Nawia/
  assets/             Game data, models, maps, textures, audio, dialogues
  docs/
    guides/           Practical architecture and workflow guides
    ET systems/       Engineering Thesis telemetry and agent system docs
  external/           Vendored dependencies
  scripts/            Utility scripts
  src/
    audio/            Sound effects, music, playlists, audio settings
    core/             Engine, camera, input, saves, resources, ET systems
    entity/           Entities, actors, abilities, effects, colliders
    item/             Items, equipment, backpack, loot tables
    ui/               HUD, menus, dialogue, inventory, quests, settings
    world/            Levels, locations, spawning, navmesh, map logic
NawiaMonitor/         PyQt telemetry monitor for combat, perception and commands
```

Useful documentation entry points:

- `Nawia/docs/guides/`
- `Nawia/docs/ET systems/Agent_Perception.md`
- `Nawia/docs/ET systems/Agent_Command_Interface.md`
- `Nawia/docs/ET systems/Boss_Raid_Mechanics.md`
- `Nawia/docs/ET systems/Combat_Event_Hooks.md`
- `Nawia/docs/ET systems/Entity_Identity_and_Damage_Context.md`
- `Nawia/docs/ET systems/Map_Pings.md`
- `Nawia/docs/ET systems/NawiaMonitor_README.md`

## Build

### Prerequisites

- C++ compiler with C++20 support
- CMake
- Git
- raylib dependencies required by the local platform
- Visual Studio developer environment when building on Windows with MSVC

### Fresh CMake Build

```powershell
git clone https://github.com/m-sadkowski/Nawia_ARPG.git
cd Nawia_ARPG/Nawia
mkdir build
cd build
cmake ..
cmake --build .
```

The executable is generated inside the selected CMake build directory.

### Existing Visual Studio/CMake Build

This repository is often built from the generated Visual Studio/CMake output
directory:

```powershell
cmake --build Nawia/out/build/x64-Release --config Release
```

When building from a plain shell on Windows, initialize the Visual Studio
developer environment first, then run the CMake build command.

## Monitor

`NawiaMonitor` is a PyQt telemetry monitor used for Engineering Thesis work. It
connects to the running game and displays best-effort runtime telemetry for
combat, agent perception, map pings, boss hazards, and agent command execution.

The monitor is useful for:

- checking what agents can currently perceive,
- inspecting combat events and boss mechanics,
- debugging command requests and command status,
- observing pings and tactical signals,
- verifying that thesis systems produce usable runtime data.

Run the monitor with:

```powershell
cd NawiaMonitor
python -m venv .venv
.\.venv\Scripts\pip install -r requirements.txt
.\.venv\Scripts\python run_monitor.py
```

The game publishes telemetry to `127.0.0.1:19777`. The monitor can be opened
before or after the game starts.

## Tech Stack

- C++20
- raylib
- CMake
- Python
- PyQt for the telemetry monitor

Copyright (c) 2025-2026 Nawia Team
