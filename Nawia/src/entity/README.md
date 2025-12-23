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

namespace Nawia::Entity {
    MyEntity::MyEntity(float x, float y, const std::shared_ptr<Texture2D>& texture)
        : Entity(x, y, texture, 100) // 100 is Max HP
    {
        // Setup 3D Model
        loadModel("../assets/models/my_model.glb");
        
        // Add Animations (map name to file)
        addAnimation("walk", "../assets/models/my_model_walk.glb");
        addAnimation("attack", "../assets/models/my_model_attack.glb");
        
        playAnimation("default"); // Plays the base model animation
    }

    void MyEntity::update(float dt) {
        Entity::update(dt); // Crucial for animation updates!
        
        // Your logic here...
        // playAnimation("walk");
    }
}
```

### Key Methods
-   `loadModel(path)`: Loads the GLB model.
-   `addAnimation(name, path)`: Maps an animation name to a GLB file.
-   `playAnimation(name)`: Switches the current animation.
-   `setRotation(angle)`: Rotates the model (in degrees).

---

## 2. Creating a New Enemy

For enemies, inherit from `Nawia::Entity::EnemyInterface` (which inherits from `Entity`).

### Steps
1.  **Inherit** from `EnemyInterface`.
2.  **Constructor**: Pass `Core::Map*` to enable pathfinding/movement logic.
3.  **Update**: Implement AI behavior in `update`.

**Example:**
```cpp
// In Constructor
loadModel("../assets/models/orc.glb");
addAnimation("run", "../assets/models/orc_run.glb");

// In Update
moveTowardsTarget(dt);
if (isMoving) playAnimation("run");
else playAnimation("default");
```

---

## 3. Creating Abilities & Effects

The ability system consists of `Ability` (logic) and `AbilityEffect` (visual representation/projectile).

### Creating a New Ability
Inherit from `Nawia::Entity::Ability`.

**Must Override:**
-   `cast(Entity* caster, float x, float y)`: Returns a `unique_ptr` to the spawned `AbilityEffect`.

**Example:**
```cpp
std::unique_ptr<Entity> MyAbility::cast(Entity* caster, float target_x, float target_y) {
    // Calculate direction, spawn projectile
    return std::make_unique<MyProjectile>(caster->getX(), caster->getY(), ...);
}
```

### Creating a New Ability Effect
Inherit from `Nawia::Entity::AbilityEffect`. This represents the actual projectile or area of effect.

**Must Override:**
-   `onCollision(std::shared_ptr<Entity>& target)`: What happens when it hits something?

**Example:**
```cpp
void Fireball::onCollision(const std::shared_ptr<Entity>& target) {
    target->takeDamage(getDamage());
    // Spawn explosion particle?
}
```

**Constructor:**
Pass duration and damage to the base `AbilityEffect` constructor.
```cpp
Fireball::Fireball(...) : AbilityEffect(x, y, texture, 2.0f /*duration*/, 50 /*damage*/) { ... }
```
