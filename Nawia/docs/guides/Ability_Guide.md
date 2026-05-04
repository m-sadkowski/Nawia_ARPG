# Przewodnik po Ability

Ten dokument opisuje aktualny system umiejętności w projekcie Nawia: klasę `Ability`, statystyki z JSON, cooldowny oraz tworzenie efektów w świecie gry.

## 1. Czym jest `Ability`

`Ability` jest obiektem logicznym przypiętym do encji, która go używa. Sama umiejętność nie jest renderowana i nie ma fizyki. Jej zadaniem jest:

- trzymanie nazwy, ikony i statystyk,
- pilnowanie cooldownu,
- sprawdzenie typu celowania,
- stworzenie efektu, pocisku albo innej encji po użyciu.

Najważniejsze pliki:

- `src/entity/abilities/Ability.h`
- `src/entity/abilities/Ability.cpp`
- `src/entity/abilities/AbilityStats.h`
- `src/entity/abilities/AbilityEffect.h`

## 2. Typy celowania

`AbilityTargetType` mówi, jak interpretować cel:

- `POINT` - umiejętność celowana w punkt świata, np. fireball albo cięcie w stronę kursora.
- `UNIT` - umiejętność celowana w konkretną encję.
- `SELF` - umiejętność rzucana na casterze.

Typ celowania powinien być zgodny z tym, co robi `cast(...)`. Jeśli ability przyjmuje punkt, używaj `POINT`, nawet gdy potem trafienie zostanie policzone po colliderze.

## 3. Statystyki

Statystyki są przechowywane w `assets/data/abilities.json` i trafiają do `AbilityStats`.

Najczęściej używane pola:

- `damage` - obrażenia zadawane przez efekt.
- `cooldown` - czas oczekiwania przed kolejnym użyciem.
- `cast_range` - zasięg użycia.
- `hitbox_radius` - domyślny rozmiar collidera lub obszaru trafienia.
- `duration` - czas życia efektu.

Statystyki pobieraj przez:

```cpp
const AbilityStats stats = Entity::getAbilityStatsFromJson("SwordSlash");
```

## 4. Jak dodać nową umiejętność

Standardowy flow:

1. Dodaj klasę dziedziczącą po `Ability`.
2. W konstruktorze przekaż nazwę, statystyki, typ celowania i ikonę.
3. Zaimplementuj `cast(float target_x, float target_y)`.
4. Jeśli ability tworzy efekt, ustaw mu frakcję castera.
5. Po udanym użyciu odpal `startCooldown()`.
6. Dodaj ability do encji przez `addAbility(...)`, zwykle w konstruktorze albo `EntityFactory`.

## 5. Przykład

Nagłówek:

```cpp
#pragma once

#include <Ability.h>

#include <memory>

namespace Nawia::Entity {

	class SuperSlashAbility : public Ability {
	public:
		/**
		 * @brief Tworzy umiejętność cięcia z teksturą efektu i ikoną HUD.
		 */
		SuperSlashAbility(
			const std::shared_ptr<Texture2D>& slash_texture,
			const std::shared_ptr<Texture2D>& icon_texture);

		/**
		 * @brief Tworzy efekt cięcia w kierunku wskazanego punktu.
		 */
		std::unique_ptr<Entity> cast(float target_x, float target_y) override;

	private:
		std::shared_ptr<Texture2D> _slash_texture;
	};

} // namespace Nawia::Entity
```

Implementacja:

```cpp
#include "SuperSlashAbility.h"

#include <SwordSlashEffect.h>

namespace Nawia::Entity {

	SuperSlashAbility::SuperSlashAbility(
		const std::shared_ptr<Texture2D>& slash_texture,
		const std::shared_ptr<Texture2D>& icon_texture)
		: Ability(
			  "SwordSlash",
			  Entity::getAbilityStatsFromJson("SwordSlash"),
			  AbilityTargetType::POINT,
			  icon_texture),
		  _slash_texture(slash_texture)
	{
	}

	std::unique_ptr<Entity> SuperSlashAbility::cast(const float target_x, const float target_y)
	{
		if (!_caster || !isReady()) return nullptr;

		_caster->rotateTowardsCenter(target_x, target_y);
		_caster->playAnimation("attack", false, true);
		startCooldown();

		const Vector2 start_pos = _caster->getCenter();
		auto slash = std::make_unique<SwordSlashEffect>(
			start_pos.x,
			start_pos.y,
			-_caster->getRotation(),
			_slash_texture,
			_stats);

		slash->setFaction(_caster->getFaction());
		return slash;
	}

} // namespace Nawia::Entity
```

## 6. Caster i spawn efektów

Każda ability ma wskaźnik `_caster`. Ustawia go `Entity::addAbility(...)`. W `cast(...)` zakładaj, że caster może być pusty tylko w błędnej konfiguracji, więc dla bezpieczeństwa sprawdź go przy bardziej ryzykownych ability.

Efekty nie powinny być dodawane bezpośrednio do `EntityManager`. Zwykły pattern:

1. `cast(...)` zwraca `std::unique_ptr<Entity>`.
2. Encja używająca ability dodaje wynik przez `addPendingSpawn(...)`.
3. `Engine` zbiera pending spawns po update i dopina je do świata.

To ogranicza modyfikowanie listy encji w trakcie iteracji po świecie.

## 7. Frakcje

Każdy efekt bojowy powinien dostać frakcję castera:

```cpp
effect->setFaction(_caster->getFaction());
```

Dzięki temu projectile albo obszarowy efekt nie rani przypadkowo właściciela i jego sojuszników, jeśli logika kolizji filtruje po `Faction`.

## 8. Pozycja startowa

Do pocisków i melee prawie zawsze używaj:

```cpp
const Vector2 start_pos = _caster->getCenter();
```

`getX()` oraz `getY()` opisują pozycję logiczną encji, ale w walce często potrzebny jest środek hitboxa lub modelu. Użycie środka zmniejsza przypadki, w których pocisk startuje z nóg albo z krawędzi modelu.

## 9. Powiązane dokumenty

- `docs/guides/AbilityEffect_Guide.md`
- `docs/guides/Entity_Guide.md`
- `docs/guides/Enemy_Guide.md`
- `docs/guides/Coding_Standards.md`
