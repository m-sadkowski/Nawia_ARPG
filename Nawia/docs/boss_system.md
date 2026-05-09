# Boss Fight System

The boss fight system is data-driven and centered around `BossManager`. Boss definitions live in `assets/data/bosses.json`, while level JSON files place `boss_trigger` entities that start a fight.

## Key Components

### BossManager

- Loads boss definitions from `assets/data/bosses.json`.
- Preloads boss and minion entities for the active level through `preloadForLevel`.
- Starts and ends boss fights, tracks defeated bosses, rewards the player, and notifies quests.
- Applies phase multipliers, phase notifications, and optional screen flash effects.
- Cleans up spawned minions after the fight.

### BossArenaTrigger

`BossArenaTrigger` is a rectangular trigger created from level entity JSON:

```json
{
  "location": "Lesna Dolina",
  "type": "boss_trigger",
  "boss_id": "devil_lord",
  "x": 0.0,
  "y": 2.0,
  "width": 10.0,
  "height": 4.0,
  "trigger_radius": 0
}
```

The trigger starts the configured boss fight when the player enters it. It does not restart an already active fight, and `BossManager` prevents defeated bosses from being triggered again.

### Teleport Blocking

There are no dynamic arena wall entities in the current implementation. Teleports are blocked while a boss fight is active, so the player cannot leave the encounter through location transitions.

### Boss UI

`UIHandler` renders a boss health bar during an active fight. The bar shows the boss name, HP, phase markers, current phase name, and fight timer. Phase transitions can also trigger a short screen flash configured in `bosses.json`.

## Boss JSON

Boss phases can change speed and damage multipliers, show notifications, flash the screen, and spawn minions:

```json
{
  "id": "devil_lord",
  "name": "Devil Lord",
  "enemy_type": "Devil",
  "level_name": "DemoLevel",
  "max_hp": 200,
  "scale": 0.03,
  "spawn_pos": { "x": 0.0, "y": 10.0 },
  "phases": [
    {
      "hp_threshold": 1.0,
      "name": "Faza 1",
      "speed_multiplier": 1.0,
      "damage_multiplier": 1.0,
      "notification": "Devil Lord atakuje!"
    }
  ],
  "rewards": {
    "gold": 500,
    "exp": 1000,
    "items": [1, 2]
  },
  "on_player_death": "end_fight"
}
```

`level_name` must match `Level::getName()`, because preloading is level-scoped.
