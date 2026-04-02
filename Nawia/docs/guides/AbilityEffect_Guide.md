# Przewodnik po klasie AbilityEffect

Klasa `AbilityEffect` (`src/entity/abilities/AbilityEffect.h`) to wyspecjalizowana klasa `Entity` reprezentująca fizyczne efekty umiejętności, takie jak pociski, eksplozje, czy obszary działania (AoE) rzucane przez graczy lub przeciwników.

## Czym różni się od zwykłego Entity?
- Posiada `AbilityStats` (obrażenia, czas trwania).
- Posiada wbudowaną obsługę czasu życia (`Lifetime`) i wygasa samoistnie po przeterminowaniu `duration`.
- Filtruje zderzenia używając zaawansowanej, hybrydowej dwufazowej logiki 3D Mesh Collider (Broadphase + Narrowphase Mesh Intersection).

## Jak stworzyć efekt (np. Pocisk Cudu Zniszczenia)?

Tworzymy klasę dziedziczącą po `Nawia::Entity::AbilityEffect`. To tutaj, w przeciwieństwie do potworów, nadal implementujemy bazowe, dwuwymiarowe obszary "Collider" ze starszych typów, gdyż to one rzutują czy trafiamy cel strefą wybuchu!

### Krok 1: Implementacja

```cpp
#include "AbilityEffect.h"
#include "Collider.h"

namespace Nawia::Entity {

    class FireballEffect : public AbilityEffect {
    public:
        FireballEffect(float startX, float startY, const std::shared_ptr<Texture2D>& tex, const AbilityStats& stats, float targetX, float targetY)
            : AbilityEffect("FireballEffect", startX, startY, tex, stats)
        {
            // Oś rotacji (obliczanie wektora kierunkowego do rzutu izometrycznego / perspektywy wektorowej na X-Z).
            float angle = atan2(targetY - startY, targetX - startX);
            float speed = stats.projectile_speed;
            
            setVelocity(cos(angle) * speed, sin(angle) * speed);

            // Wymiar obszaru zadawania obrażeń pocisku. Koło wokół środka efektu.
            setCollider(std::make_unique<CircleCollider>(this, stats.hitbox_radius, 0.0f, 0.0f));
            
            // Opcjonalne: Użycie zaawansowanego modelu na lecącej kuli
            // loadModel("assets/models/fireball.glb");
        }

        void update(float dt) override {
            AbilityEffect::update(dt); // WAŻNE: Obsługuje ruch i obniżanie czasu wygaśnięcia pocisku
        }

        // Obsługa trafienia
        void onCollision(const std::shared_ptr<Entity>& target) override {
            // Klasa bazowa dba by nie doliczać hitboxów po dziesięć razy dla jednej fazy cięcia mieczem
            if (hasHit(target)) return;
            
            // Reakcja: Zadaj obrażenia
            target->takeDamage(_stats.damage);
            addHit(target); // Zapisz, że ten cel przyjął strzał
            
            // Ponieważ To fireball, ulegnie destrukcji
            die();
        }
    };
}
```

## Kluczowe Procesy Kolizji Walki: Dwuetapowa Weryfikacja

Twoje efekty wyłapują interakcje ze wrogami korzystając z precyzyjnej mechaniki `checkCollision(target)`. Od migracji na natywny układ głębi 3D robimy to zaawansowanymi metodami silnika, unikając mylnych zderzeń z rogiem powietrza!

### 1. Krok: Broadphase (`BoundingBox` spięcie z `Colliderem`)
Efekt w pierwszej fazie próbuje dociec, czy w ogóle zderzył się z animacyjnym polem brzegowym celu (Wbudowane pudło na modelach objętościowych potworów z Blendera). Jeżeli szeroki ConeCollider (Strefa miecza) chociaż ułamkiem musnęła Boxa Ogra, przechodzi do Narrowphase.

### 2. Krok: Narrowphase (`Mesh Intersection Raycasting`)
Wiązki punktów rzutowane są od epicentrum efektu w siatkę wroga (`getRayMeshCollision()`). Jeśli miecz uderzył celnie siatkę geometryczną przeciwnika na poprawnej wielkości osi, `checkCollision` zezwala wywołać reakcję zadania bólu!

### `onCollision(target)`
Wywoływana przez silnik, gdy przejdą obie fazy sprawdzenia z wrogiem. Tutaj definiujesz konsekwencje (Obrażenia, efekty obezwładniania, eksplozje).

## System Kolizji i Frakcji (Szczegóły)
- Pamiętaj, aby ustawić Frakcję pocisku taką samą jak Castera przy wykluwaniu jego jajka w klasie macierzy umiejętności `SuperSlashBase->cast(...)` - tak unika rażenia przyjaciół (`Faction::Ally`).
- Efekty mogą zadawać trafienia strefą koła (`CircleCollider`) wokół promienia rzutu dla pocisków, i strefą stożka (`ConeCollider`) idealnie pasującą pod frontowe ataki w walce wręcz!
