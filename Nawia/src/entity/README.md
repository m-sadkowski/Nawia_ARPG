# System Entity - Przewodnik Dewelopera

Ten dokument stanowi kompletny przewodnik po systemie Entity w projekcie Nawia. Opisuje hierarchię klas, system kolizji, tworzenie nowych przeciwników (Enemies) oraz umiejętności (Abilities) i ich efektów.

## Spis Treści
1.  [Hierarchia Klas](#1-hierarchia-klas)
2.  [Tworzenie Nowego Entity](#2-tworzenie-nowego-entity)
    *   [Konstrukcja i Modele](#konstrukcja-i-modele-3d)
    *   [Pozycjonowanie i Rotacja](#pozycjonowanie-i-rotacja)
3.  [System Kolizji (Collider)](#3-system-kolizji-collider)
4.  [Tworzenie Przeciwników (Enemies)](#4-tworzenie-przeciwników-enemies)
5.  [System Umiejętności (AbILITIES)](#5-system-umiejętności-abilities)
    *   [Konfiguracja JSON](#51-konfiguracja-json)
    *   [Klasa Ability](#52-klasa-ability)
    *   [AbilityEffect (Pociski/Efekty)](#53-abilityeffect-pociskiefekty)

---

## 1. Hierarchia Klas

Podstawowym budulcem jest klasa `Nawia::Entity::Entity`.

*   **Entity**: Klasa bazowa. Posiada pozycję, teksturę/model, życie (HP), system animacji i kolizji.
*   **EnemyInterface**: Dziedziczy po `Entity`. Rozszerza bazę o wskaźnik na `Map` (dla pathfindingu) i logikę specyficzną dla wrogów.
*   **Ability**: Klasa logiczna umiejętności (nie jest Entity, ale jest trzymana przez Entity). Odpowiada za "rzucenie" czaru (funkcja `cast`).
*   **AbilityEffect**: Dziedziczy po `Entity`. Reprezentuje wizualny i fizyczny efekt umiejętności (np. kula ognia, cięcie mieczem).

---

## 2. Tworzenie Nowego Entity

Aby stworzyć nowy obiekt w grze, stwórz klasę dziedziczącą po `Entity`.

### Konstruktor

Konstruktor bazowy wymaga podstawowych danych:
```cpp
Entity(
    const std::string& name,                // Debugowa nazwa
    float start_x, float start_y,           // Pozycja w świecie (siatka izometryczna)
    const std::shared_ptr<Texture2D>& texture, // Tekstura 2D (ikona/sprite)
    int max_hp                              // Maksymalne życie
);
```

### Konstrukcja i Modele 3D
W ciele konstruktora swojej klasy powinieneś załadować model 3D oraz animacje.

```cpp
// MyEntity.cpp
MyEntity::MyEntity(float x, float y, const std::shared_ptr<Texture2D>& tex)
    : Entity("MyEntity", x, y, tex, 100)
{
    // Ładowanie modelu 3D
    // Drugi parametr 'true' obraca model o -90 stopni na osi X (częste dla modeli z Blendera/Z-up)
    loadModel("../assets/models/barrel.glb", true);
    
    // Rejestracja animacji
    addAnimation("idle", "../assets/models/barrel_idle.glb");
    
    // Uruchomienie domyślnej animacji
    playAnimation("idle");
}
```

### Pozycjonowanie i Rotacja

W grze izometrycznej rozróżniamy pozycję "stóp" (gdzie postać stoi na ziemi) od "środka" (gdzie postać powinna oberwać pociskiem).

*   `getX()`, `getY()`: Zwraca pozycję 2D na mapie (podstawa modelu).
*   `getCenter()`: Zwraca `Vector2`, który jest środkiem geometrycznym (często środkiem Collidera). **Używaj tego do celowania w tę postać.**

#### Rotacja Postaci
Entity posiada dwie metody do obracania się, w zależności od kontekstu:

1.  **`rotateTowards(x, y)`** - Rotacja względem stóp (bazy).
    *   Używane przy **chodzeniu** (postać idzie tam, gdzie wskazują stopy).
2.  **`rotateTowardsCenter(x, y)`** - Rotacja względem środka (`getCenter()`).
    *   Używane przy **walce i celowaniu** (postać obraca korpus i twarz w stronę celu/kursora).

> **Ważne:** Jeśli tworzysz umiejętność, która wymaga precyzyjnego celowania, użyj `rotateTowardsCenter` w momencie rzucania (cast), aby upewnić się, że pocisk poleci dokładnie z "klatki piersiowej" w stronę kursora.

---

## 3. System Kolizji (Collider)

Collider definiuje fizyczny kształt obiektu. Jest kluczowy dla wykrywania trafień i interakcji myszką. Kolizje są dodawane w konstruktorze za pomocą `setCollider`.

### Typy Colliderów:

1.  **`RectangleCollider`**: Prostokąt. Standard dla postaci.
    ```cpp
    // (właściciel, szerokość, wysokość)
    setCollider(std::make_unique<RectangleCollider>(this, 0.5f, 0.8f));
    ```

2.  **`CircleCollider`**: Koło. Standard dla pocisków typu Fireball.
    ```cpp
    // (właściciel, promień)
    setCollider(std::make_unique<CircleCollider>(this, 0.5f));
    ```

3.  **`ConeCollider`**: Stożek/Wycinek koła. Dla ataków obszarowych (Cleave/Slash).
    ```cpp
    // (właściciel, zasięg/promień, kąt widzenia w stopniach)
    setCollider(std::make_unique<ConeCollider>(this, 1.5f, 90.0f));
    ```

> Aby widzieć hitboxy w grze, ustaw `Entity::DebugColliders = true;` w kodzie.

---

## 4. Tworzenie Przeciwników (Enemies)

Przeciwnicy powinni dziedziczyć po `Nawia::Entity::EnemyInterface`, nie po pustym `Entity`. Klasa ta zapewnia integrację z systemem mapy.

### Przykład: Orc
```cpp
// Orc.h
class Orc : public Nawia::Entity::EnemyInterface {
public:
    Orc(float x, float y, std::shared_ptr<Texture2D> tex, Nawia::Core::Map* map);
    void update(float dt) override;
};
```

```cpp
// Orc.cpp
Orc::Orc(float x, float y, std::shared_ptr<Texture2D> tex, Nawia::Core::Map* map)
    : EnemyInterface("Orc", x, y, tex, 200, map) // 200 HP
{
    loadModel("../assets/models/orc.glb", true);
    addAnimation("run", "../assets/models/orc_run.glb");
    addAnimation("attack", "../assets/models/orc_attack.glb");
    
    // Ważne: Collider ustawiamy tak, żeby pasował do modelu
    setCollider(std::make_unique<RectangleCollider>(this, 0.6f, 0.8f));
}

void Orc::update(float dt) {
    EnemyInterface::update(dt); // Obsługa fizyki i animacji bazowej
    
    // Prosta logika AI
    // if (player_is_close) { ... }
}
```

---

## 5. System Umiejętności (Abilities)

System ten pozwala oddzielić dane (JSON) od logiki (C++).

### 5.1. Konfiguracja JSON
Dodaj wpis do `assets/data/abilities.json`. To tutaj definiujesz zasięg, obrażenia i cooldowny.

```json
{
  "name": "SuperSlash",
  "stats": {
    "damage": 50,
    "cooldown": 1.5,
    "cast_range": 2.0,
    "hitbox_radius": 2.5,
    "duration": 0.2
  }
}
```

### 5.2. Klasa Ability
Odpowiada za logikę "użycia". Musisz stworzyć klasę dziedziczącą po `Ability`.

**Kluczowa metoda: `cast()`**
To tutaj decydujesz co się dzieje. Czy spawnuje się pocisk? Czy postać musi się obrócić?

```cpp
std::unique_ptr<Entity> SuperSlashAbility::cast(float target_x, float target_y) 
{
    // 1. Wymuś rotację w stronę celu (szczególnie ważne przy sterowaniu klawiaturą Q/W/E/R)
    getCaster()->rotateTowardsCenter(target_x, target_y);

    // 2. Uruchom cooldown i animację postaci
    startCooldown();
    getCaster()->playAnimation("attack", false, true); // true = blokuje ruch na czas ataku

    // 3. Stwórz efekt (hitbox zadający obrażenia)
    // Pobierz statystyki z _stats (załadowane z JSON)
    // Kąt w tym przypadku to -getCaster()->getRotation()
    auto slash = std::make_unique<SwordSlashEffect>(
        getCaster()->getCenter().x, 
        getCaster()->getCenter().y, 
        -getCaster()->getRotation(), 
        _texture, 
        _stats
    );
    
    return slash;
}
```

### 5.3. AbilityEffect (Pociski/Efekty)
To jest fizyczna reprezentacja ataku w świecie gry. Dziedziczy po `AbilityEffect` (który dziedziczy po `Entity`).

Musi implementować:
*   `onCollision`: Zadawanie obrażeń.
*   `render`: Rysowanie (tekstura lub collider).

```cpp
void SwordSlashEffect::onCollision(const std::shared_ptr<Entity>& target)
{
    // Rzutowanie na EnemyInterface (żeby nie bić skrzynek czy sojuszników, chyba że chcemy)
    if (auto enemy = std::dynamic_pointer_cast<EnemyInterface>(target))
    {
        // Sprawdź czy już go nie trafiliśmy w tym samym ataku (żeby nie zadać obrażeń co klatkę)
        if (!hasHit(enemy)) {
            enemy->takeDamage(getDamage());
            addHit(enemy); // Dodaj do listy trafionych
        }
    }
}
```
