# Dokumentacja Systemu Umiejętności (Abilities)

## Przegląd Architektury

System umiejętności opiera się na dwóch uzupełniających się klasach bazowych znajdujących się w `src/entity/abilities/`:

1.  **`Ability`** (`Ability.h`):
    *   Reprezentuje definicję umiejętności (dane statyczne, cooldown, zasięg).
    *   Odpowiada za logikę **użycia** (rzucenia) czaru.
    *   Jest przechowywana w kontrolerze postaci (`PlayerController`).

2.  **`AbilityEffect`** (`AbilityEffect.h`):
    *   Reprezentuje instancję efektu w świecie gry (np. lecąca kula ognia, cięcie mieczem).
    *   Jest pełnoprawnym bytem (`Entity`), zarządznym przez `EntityManager`.
    *   Posiada czas trwania (`duration`), po którym znika.
    *   Odpowiada za **logikę kolizji** i **aplikowanie efektów** (np. obrażeń) na celach.
---

## 1. Jak stworzyć nową umiejętność (`Ability`)

Aby dodać nową umiejętność, należy stworzyć nową klasę dziedziczącą publicznie po `Nawia::Entity::Ability`.

### Krok 1: Konstruktor
W konstruktorze nowej klasy musisz wywołać konstruktor bazowy `Ability`, przekazując następujące parametry:
*   `name` (string): Nazwa wyświetlana umiejętności (np. "Sword Slash").
*   `cooldown` (float): Czas odnowienia w sekundach.
*   `cast_range` (float): Maksymalny zasięg użycia.
*   `target_type` (`AbilityTargetType`): Typ celowania.

**Dostępne typy celowania (`AbilityTargetType`):**
*   `POINT`: Gracz wskazuje miejsce kurorem na ekranie (np. Fireball, szarża).
*   `UNIT`: Wymaga kliknięcia bezpośrednio na wroga.
*   `SELF`: Używane natychmiast na sobie (np. leczenie, buff).

### Krok 2: Nadpisanie metody `cast`
Musisz zaimplementować czysto wirtualną metodę:
```cpp
virtual std::unique_ptr<Entity> cast(Entity* caster, float target_x, float target_y) override;
```
*   **Kiedy jest wywoływana?**: Gdy gracz naciśnie przycisk i spełnione są warunki (cooldown, zasięg).
*   **Co ma robić?**:
    1.  Wywołać `startCooldown()` (aby zresetować licznik).
    2.  Obliczyć pozycję startową efektu (np. przed graczem, w miejscu kursora).
    3.  Stworzyć i zwrócić instancję odpowiedniego `AbilityEffect`.

---

## 2. Jak stworzyć nowy efekt (`AbilityEffect`)

Efekt wizualny i logiczny umiejętności musi dziedziczyć po `Nawia::Entity::AbilityEffect`.

### Krok 1: Konstruktor
Wywołaj konstruktor bazowy `AbilityEffect`:
```cpp
AbilityEffect(float x, float y, const std::shared_ptr<SDL_Texture>& tex, float duration, int damage);
```
*   `duration`: Jak długo efekt istnieje w świecie (w sekundach). Po upływie tego czasu `isExpired()` zwróci true i `EntityManager` usunie obiekt.
*   `damage`: Bazowa ilość obrażeń.

### Krok 2: Nadpisywanie zachowań (Metody wirtualne)

W zależności od potrzeb, możesz nadpisać następujące metody:

#### A. Ruch i aktualizacja (`update`)
Jeśli efekt ma się poruszać (np. pocisk):
```cpp
void update(float dt) override {
    AbilityEffect::update(dt); // WAŻNE: Aktualizuje timer czasu życia!
    // Tu dodaj logikę ruchu, np.:
    // _pos->x += velocity_x * dt;
}
```

#### B. Rysowanie (`render`)
Domyślna metoda `render` rysuje teksturę w aktualnej pozycji. Nadpisz ją tylko, jeśli potrzebujesz:
*   Rotacji tekstury (np. cięcie mieczem zależne od kąta ataku).
*   Animacji klatkowej.
*   Efektów cząsteczkowych.

#### C. Kolizje (`checkCollision` i `onCollision`)
To najważniejsza część logiki bojowej.

1.  **`bool checkCollision(const std::shared_ptr<Entity>& target) const`**
    *   Tutaj definiujesz **kształt** obszaru rażenia.
    *   Sprawdź, czy cel jest wrogiem (`dynamic_pointer_cast<Enemy>`).
    *   Sprawdź, czy `!hasHit(target)` (chyba że umiejętność ma razić wielokrotnie co klatkę).
    *   Oblicz kolizję geometryczną (dystans, stożek, prostokąt).
    *   Zwróć `true`, jeśli trafiono.

2.  **`void onCollision(const std::shared_ptr<Entity>& target)`**
    *   Wywoływana tylko, gdy `checkCollision` zwróci `true`.
    *   Tutaj aplikujesz efekty (obrażenia, odrzucenie, statusy).
    *   **Zawsze** wywołaj `addHit(target)`, aby zapobiec natychmiastowemu ponownemu trafieniu w następnej klatce (dla umiejętności jednorazowych).

    ```cpp
    void onCollision(const std::shared_ptr<Entity>& target) override {
        if (auto enemy = std::dynamic_pointer_cast<Enemy>(target)) {
            enemy->takeDamage(getDamage());
            addHit(enemy); // Oznacz jako trafiony
        }
    }
    ```

---

## Podsumowanie procesu dodawania umiejętności

1.  Stwórz klasę `MySkillEffect` : `AbilityEffect`.
    *   Zaimplementuj kształt kolizji w `checkCollision`.
    *   Ustaw obrażenia w `onCollision`.
2.  Stwórz klasę `MySkillAbility` : `Ability`.
    *   W `cast` stwórz instancję `MySkillEffect`.
3.  Dodaj nową umiejętność do `PlayerController` (np. w `init()` lub konstruktorze).

## Dobre praktyki
*   Pamiętaj o `startCooldown()` w metodzie `cast`.
*   Używaj `render_diffs` lub loggera do debugowania hitboxów, jeśli kolizje nie działają.
*   Pamiętaj, że `AbilityEffect` jest usuwany automatycznie po czasie `duration`.
