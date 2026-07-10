# Nawia: Slavic Action RPG

Nawia is an isometric action RPG built in C++ with raylib. The project combines
custom engine code, data-driven gameplay, boss encounters, dialogue, quests,
level tooling, and a dedicated telemetry monitor used for Engineering Thesis
experiments.

The current technical direction is not a full ECS. Runtime gameplay is centered
around `Engine`, `EntityManager`, `Entity`, actor/interactable subclasses,
data-driven factories, and focused manager systems.

## Academic Context

The project is developed at Gdansk University of Technology and is being used as
a foundation for Engineering Thesis work around believable fake multiplayer:
limited agent perception, tactical communication, role behavior, imperfect human
decision-making, support, retreat, grouping, and boss mechanics.

## Main Modules

```text
Nawia/
  assets/             Game data, models, maps, textures, audio
  docs/
    guides/           Practical architecture and workflow guides
    ET systems/       Engineering Thesis telemetry/agent infrastructure docs
  external/           Vendored dependencies
  scripts/            Utility scripts
  src/
    audio/            Sound and music management
    core/             Engine, camera, input, map, saves, ET systems
    entity/           Entities, actors, abilities, effects, colliders
    item/             Items, equipment, backpack, loot tables
    ui/               HUD, menus, dialogue, inventory, quest UI
    world/            Levels, locations, spawning, navmesh
NawiaMonitor/         PyQt telemetry monitor for combat, perception and commands
```

## ET Infrastructure

The ET layer currently provides:

- stable runtime `entity_id` values,
- combat event hooks,
- agent perception snapshots,
- map pings,
- boss cast and hazard telemetry,
- agent command execution/status tracking,
- localhost NDJSON telemetry,
- a PyQt monitor for debugging combat, perception and command state.

Start with:

- `Nawia/docs/ET systems/Agent_Perception.md`
- `Nawia/docs/ET systems/Agent_Command_Interface.md`
- `Nawia/docs/ET systems/NawiaMonitor_README.md`

## Build

The repository is typically built from the generated Visual Studio/CMake output
directory:

```powershell
cmake --build Nawia/out/build/x64-Release --config Release
```

When building from a plain shell on Windows, use the Visual Studio developer
environment first.

## Monitor

```powershell
cd NawiaMonitor
python -m venv .venv
.\.venv\Scripts\pip install -r requirements.txt
.\.venv\Scripts\python run_monitor.py
```

The game publishes best-effort telemetry to `127.0.0.1:19777`; the monitor can
be opened before or after the game starts.
