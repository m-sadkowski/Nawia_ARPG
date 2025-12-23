# Entity System Documentation

This folder contains the core logic for game entities, including actors (Player, Enemies), abilities, and effects.

## 1. Creating a New Entity

To create a new game object, inherit from the `Nawia::Entity::Entity` class.

### Basic Setup
1.  **Inherit**: Create a class that inherits from `Nawia::Entity::Entity`.
2.  **Constructor**: Call the base `Entity` constructor.
3.  **3D Model & Animation**: Use `loadModel` and `addAnimation` in your constructor to enable 3D rendering.

**Example Header (.h):**
```cpp
#pragma once
#include "Entity.h"

namespace Nawia::Entity {
    class MyEntity : public Entity {
    public:
        MyEntity(float x, float y, const std::shared_ptr<Texture2D>& texture);
        void update(float dt) override;
    };
}
```

**Example Implementation (.cpp):**
```cpp
#include "MyEntity.h"
#include "Constants.h" // For TILE_WIDTH, etc.
#include <MathUtils.h> // For generic math

namespace Nawia::Entity {
    MyEntity::MyEntity(float x, float y, const std::shared_ptr<Texture2D>& texture)
        : Entity(x, y, texture, 100) // 100 is Max HP
    {
        // Setup 3D Model
        loadModel("../assets/models/my_model.glb");
        
        // Add Animations (map name to file)
        addAnimation("walk", "../assets/models/my_model_walk.glb");
        
        playAnimation("default"); // Plays the base model animation
        
        // Transform attributes
        setVelocity(10.0f, 0.0f);
        setScale(1.5f);
    }

    void MyEntity::update(float dt) {
        Entity::update(dt); // Crucial for animation updates and velocity integration!
        
        // Your logic here...
    }
}
```

---

## 2. Creating a New Enemy

For enemies, inherit from `Nawia::Entity::EnemyInterface` (which inherits from `Entity`).

### Steps
1.  **Inherit** from `EnemyInterface`.
2.  **Constructor**: Pass `Core::Map*` to enable pathfinding/movement logic.
3.  **Update**: Implement AI behavior in `update`.

**Example Implementation:**
```cpp
#include "MyEnemy.h"
#include "Constants.h"
#include <cmath>

namespace Nawia::Entity {

    MyEnemy::MyEnemy(float x, float y, const std::shared_ptr<Texture2D>& tex, int max_hp, Core::Map* map)
        : EnemyInterface(x, y, tex, max_hp, map)
    {
        loadModel("../assets/models/orc.glb");
        addAnimation("run", "../assets/models/orc_run.glb");
        playAnimation("default");
    }

    void MyEnemy::update(float dt) {
        EnemyInterface::update(dt);
        
        // Example: Simple chase logic
        // ... (calculate dist/angle) ...
        
        if (is_moving) {
             playAnimation("run");
             // Update position...
        } else {
             playAnimation("default");
        }
    }
}
```

---

## 3. Creating Abilities & Effects

The ability system consists of `Ability` (logic), `AbilityEffect` (visual/projectile), and `AbilityStats` (data).

### Ability Stats (JSON Configuration)
Abilities should not hardcode their statistics (damage, range, etc.). Instead, define them in `assets/data/abilities.json`.

**JSON Format:**
```json
{
  "abilities": [
    {
      "name": "My Ability",
      "stats": {
        "damage": 50,
        "cooldown": 2.5,
        "cast_range": 15.0,
        "projectile_speed": 12.0,
        "duration": 5.0,
        "hitbox_radius": 1.5
      }
    }
  ]
}
```

### Loading Stats
Use the helper function `Entity::getAbilityStatsFromJson(name)` in your `Ability` constructor.

### Creating a New Ability
Inherit from `Nawia::Entity::Ability`.

**Constructor:**
Pass the loaded stats to the base constructor.

```cpp
MyAbility::MyAbility(...) 
    : Ability("My Ability", Entity::getAbilityStatsFromJson("My Ability"), AbilityTargetType::UNIT) 
{ ... }
```

**Must Override:**
-   `cast(Entity* caster, float x, float y)`: Returns a `unique_ptr` to the spawned `AbilityEffect`.

### Creating a New Ability Effect
Inherit from `Nawia::Entity::AbilityEffect`. This represents the actual projectile or area of effect.

**Constructor:**
Pass the `AbilityStats` object to the base constructor.

```cpp
MyEffect::MyEffect(..., const AbilityStats& stats) 
    : AbilityEffect(x, y, texture, stats) 
{ ... }
```

**Must Override:**
-   `onCollision(std::shared_ptr<Entity>& target)`: What happens when it hits something?

---

## 4. Tips & Troubleshooting

-   **New Files & CMake**: If you create a new `.cpp` file, you might need to "touch" (add a space/save) the `CMakeLists.txt` file or re-run CMake manually so it picks up the new source file.
-   **Headers**: 
    -   Include `"Constants.h"` if you need `Core::TILE_WIDTH` or `Core::pi`.
    -   Include `<MathUtils.h>` for utility math functions.
-   **Coordinates**: `Entity` positions are generally in World Coordinates.
