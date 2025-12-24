# System Entity - Przewodnik Dewelopera

Ten dokument opisuje jak tworzyć nowe obiekty w grze (Entities), Przeciwników (Enemies) oraz Umiejętności (Abilities) w projekcie Nawia.

## 1. Tworzenie nowego Entity (Obiektu Gry)

Wszystkie obiekty w grze dziedziczą po klasie `Nawia::Entity::Entity`.

### Krok po kroku:
1.  Stwórz klasę dziedziczącą po `Nawia::Entity::Entity`.
2.  Wywołaj konstruktor bazowy `Entity`.
3.  W konstruktorze załaduj model 3D i animacje (opcjonalnie).

### Konstruktor Entity
Musisz przekazać następujące parametry do konstruktora bazowego:
```cpp
Entity(
    const std::string& name,                // Unikalna nazwa (np. "Chest")
    float start_x, float start_y,           // Pozycja początkowa
    const std::shared_ptr<Texture2D>& texture, // Tekstura 2D (ikona/sprite)
    int max_hp                              // Życie
);
```

### Przykład:
```cpp
// MyEntity.h
class MyEntity : public Nawia::Entity::Entity {
public:
    MyEntity(float x, float y, const std::shared_ptr<Texture2D>& tex)
        : Entity("MyEntity", x, y, tex, 100) // 100 HP
    {
        // Ładowanie modelu 3D
        loadModel("../assets/models/barrel.glb");
        
        // Dodawanie animacji (nazwa, ścieżka)
        addAnimation("idle", "../assets/models/barrel_idle.glb");
        playAnimation("idle");
    }
};
```

```

---

## 2. System Kolizji (Collider System)

Każde Entity może posiadać przypisany `Collider`, który definiuje jego fizyczny kształt w świecie gry. System wspiera różne typy kształtów i zapewnia mechanizmy wykrywania kolizji oraz debugowania.

### Dostępne Typy Colliderów:

1.  **RectangleCollider**: Prostokąt, definiowany przez szerokość i wysokość. Idealny dla postaci i większości obiektów.
2.  **CircleCollider**: Koło, definiowane przez promień. Używany np. dla pocisków.
3.  **ConeCollider**: Stożek (wycinek koła), definiowany przez promień i kąt. Używany dla ataków obszarowych (jak Sword Slash).

### Jak dodać Collider do Entity?

W konstruktorze swojego Entity użyj metody `setCollider`:

```cpp
// Przykłady:

// 1. RectangleCollider (np. dla Gracza/Przeciwnika)
// Parametry: właściciel (this), szerokość, wysokość
setCollider(std::make_unique<RectangleCollider>(this, 0.5f, 0.5f));

// 2. CircleCollider (np. dla Kuli Ognia)
// Parametry: właściciel (this), promień
setCollider(std::make_unique<CircleCollider>(this, 0.5f));

// 3. ConeCollider (np. dla ataku mieczem)
// Parametry: właściciel (this), promień, kąt (w stopniach)
setCollider(std::make_unique<ConeCollider>(this, 1.5f, 90.0f));
```

### Debugowanie
Aby zobaczyć hitboxy w grze, można ustawić flagę statyczną `Entity::DebugColliders` na `true`.
- **Czerwony**: CircleCollider
- **Niebieski**: RectangleCollider
- **Zielony**: ConeCollider

---

## 2. Tworzenie nowego Przeciwnika (Enemy)

Przeciwnicy dziedziczą po `Nawia::Entity::EnemyInterface`, który zapewnia im dostęp do mapy i podstawową logikę ruchu.

### Krok po kroku:
1.  Stwórz klasę dziedziczącą po `EnemyInterface`.
2.  Zaimplementuj metodę `update`, aby dodać sztuczną inteligencję (AI).

### Konstruktor EnemyInterface
```cpp
EnemyInterface(
    const std::string& name, 
    float x, float y, 
    const std::shared_ptr<Texture2D>& texture, 
    int max_hp, 
    Core::Map* map // Wskaźnik na mapę dla pathfindingu
);
```

### Przykład:
```cpp
// Orc.cpp
Orc::Orc(float x, float y, std::shared_ptr<Texture2D> tex, Core::Map* map)
    : EnemyInterface("Orc", x, y, tex, 200, map)
{
    loadModel("../assets/models/orc.glb");
    addAnimation("run", "../assets/models/orc_run.glb");
}

void Orc::update(float dt) {
    EnemyInterface::update(dt);
    // Twoja logika AI tutaj (np. gonienie gracza)
}
```

---

## 3. Tworzenie Umiejętności (Abilities)

System umiejętności składa się z dwóch części:
1.  **Dane** (JSON): Statystyki umiejętności.
2.  **Logika** (C++): Klasa dziedzicząca po `Ability`.

### 3.1. Konfiguracja JSON (`assets/data/abilities.json`)
Nie wpisuj statystyk na sztywno w kodzie. Dodaj je do pliku JSON.

```json
{
  "abilities": [
    {
      "name": "Fireball",
      "stats": {
        "damage": 50,
        "cooldown": 2.0,
        "cast_range": 10.0,
        "projectile_speed": 15.0,
        "duration": 5.0,
        "hitbox_radius": 1.0
      }
    }
  ]
}
```

### 3.2. Klasa Ability
Stwórz klasę dziedziczącą po `Nawia::Entity::Ability`.

W konstruktorze użyj `Entity::getAbilityStatsFromJson("NazwaZJsona")`, aby pobrać statystyki.

**Ważne:** Musisz nadpisać metodę `cast`.

```cpp
// FireballAbility.cpp
FireballAbility::FireballAbility()
    : Ability("Fireball", Entity::getAbilityStatsFromJson("Fireball"), AbilityTargetType::POINT)
{
}

std::unique_ptr<Entity> FireballAbility::cast(float target_x, float target_y) {
    // Zwróć nowy efekt/pocisk
    // Użyj getCaster(), aby przekazać stwórcę (żeby nie trafił sam siebie)
    return std::make_unique<FireballProjectile>(
        getCaster()->getX(), getCaster()->getY(), 
        target_x, target_y, 
        _stats, // Statystyki załadowane z JSON
        getCaster()
    );
}
```

---

## 4. Tworzenie Efektów / Pocisków (AbilityEffect)

Efekt umiejętności (np. kula ognia) to osobny obiekt dziedziczący po `AbilityEffect` (lub `Projectile`, jeśli to pocisk).

### Konstruktor AbilityEffect
```cpp
AbilityEffect(
    const std::string& name,
    float x, float y,
    const std::shared_ptr<Texture2D>& tex,
    const AbilityStats& stats
);
```

### Nadpisz te funkcje:
*   `onCollision(std::shared_ptr<Entity>& target)`: Co się dzieje, gdy w coś trafi?
*   `update(float dt)`: Logika lotu / trwania.

### Przykład (Pocisk):
```cpp
// FireballProjectile.cpp
void FireballProjectile::onCollision(std::shared_ptr<Entity>& target) {
    if (target.get() == _caster) return; // Nie rań strzelającego
    
    target->takeDamage(_stats.damage);
    _hp = 0; // Zniszcz pocisk po trafieniu
}
```

---

## 5. Wskazówki dla Deweloperów

1.  **Rejestracja Entity**: Jeśli stworzysz nowy obiekt ręcznie (np. w `main.cpp`), MUSISZ go dodać do `EntityManager` używając `addEntity()`. Inaczej nie będzie on aktualizowany, renderowany ani brany pod uwagę w kolizjach.
2.  **Konwersja Pozycji**: Pamiętaj, że logika gry używa współrzędnych świata (World Coordinates), a nie ekranu.
3.  **CMake**: Pamiętaj, żeby odświeżyć CMake po dodaniu nowych plików `.cpp` / `.h`.
