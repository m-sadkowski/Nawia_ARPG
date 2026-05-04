# Boss Fight System

The Boss Fight System is a data-driven framework for creating epic encounters in Nawia. It manages arena locking, boss spawning, UI progression, and rewards.

## Key Components

### 1. BossManager
The central hub for all boss encounters.
- **Loading**: Loads boss definitions from `assets/data/bosses.json`.
- **Fight Lifecycle**: Handles `startBossFight` and `endBossFight`.
- **Arena Management**: Automatically spawns `BossWall` entities to lock the player in the arena.
- **Entity Management**: Spawns the boss entity based on the `enemy_type` defined in JSON.
- **Rewards**: Automatically distributes XP, gold, and items upon boss defeat.

### 2. BossData (JSON Schema)
Bosses are defined in `bosses.json`.
```json
{
  "id": "devil_lord",
  "name": "Devil Lord",
  "enemy_type": "Devil",
  "level_name": "DemoLevel",
  "max_hp": 1500,
  "scale": 1.5,
  "arena": {
    "x": -10.0, "y": 5.0, "width": 20.0, "height": 15.0
  },
  "spawn_pos": {
    "x": 0.0, "y": 12.0
  },
  "rewards": {
    "gold": 500,
    "exp": 1000,
    "items": [1, 2]
  }
}
```

### 3. BossArenaTrigger
A specialized trigger entity that starts the fight when the player enters.
- Defined in level entity JSONs (e.g., `demo_level.json`).
- Links to a specific `boss_id`.

### 4. BossWall
Invisible (in production) or debug-visible walls that use the `EntityType::Wall` to block movement.
- Spawned dynamically by `BossManager`.
- Cleaned up automatically after the fight.

### 5. Boss UI
A dedicated Boss Health Bar is rendered at the top of the screen when a fight is active.
- Managed by `UIHandler`.
- Displays boss name and precise HP values.

## Implementation Details

- **Collision**: `EntityManager` was updated to handle `Wall` entity collisions, preventing players and enemies from leaving the arena.
- **Dynamic Spawning**: Bosses are spawned using Builders (e.g., `DevilBuilder`) to ensure they have the correct stats and scale.
- **Thesis Interface**: The system is modular and decoupled. `BossManager` only interacts with `Engine` and `EntityManager`, making it easy to extend with new boss types or mechanics.
