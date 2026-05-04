# Przewodnik po Ability

Ten dokument opisuje aktualny system umiejętności w projekcie Nawia: klasę `Ability`,
statystyki z JSON, czas odnowienia oraz tworzenie efektów w świecie gry.

## 1. Czym jest `Ability`

`Ability` jest obiektem logicznym przypiętym do encji, która go używa. Sama
umiejętność nie jest renderowana i nie ma fizyki. Jej zadaniem jest:

- trzymanie nazwy, ikony i statystyk,
- pilnowanie czasu odnowienia,
- sprawdzenie typu celowania,
- stworzenie efektu, pocisku albo innej encji po użyciu.

Najważniejsze pliki:

- `src/entity/abilities/Ability.h`
- `src/entity/abilities/Ability.cpp`
- `src/entity/abilities/AbilityStats.h`
- `src/entity/abilities/AbilityEffect.h`
- `src/entity/abilities/projectiles/ProjectileAbility.h`

## 2. Typy celowania

`AbilityTargetType` mówi, jak interpretować cel:

- `POINT` - umiejętność celowana w punkt świata.
- `UNIT` - umiejętność celowana w konkretną encję.
- `SELF` - umiejętność rzucana na źródło użycia.

Typ celowania powinien być zgodny z tym, co robi `cast(...)`. Jeśli ability
przyjmuje punkt, używaj `POINT`, nawet gdy potem trafienie zostanie policzone
po koliderze.

## 3. Statystyki

Statystyki są przechowywane w `assets/data/abilities.json` i trafiają do
`AbilityStats`.

Najczęściej używane pola:

- `damage` - obrażenia zadawane przez efekt.
- `cooldown` - czas oczekiwania przed kolejnym użyciem.
- `cast_range` - zasięg użycia.
- `hitbox_radius` - domyślny rozmiar kolidera lub obszaru trafienia.
- `duration` - czas życia efektu.
- `projectile_speed` - prędkość pocisku.

Statystyki pobieraj przez:

```cpp
const AbilityStats stats = Entity::getAbilityStatsFromJson("SwordSlash");
```

## 4. Kontrakt castowania

`cast(float target_x, float target_y)` zwraca `AbilitySpawn`, czyli
`std::shared_ptr<Entity>`.

Zasady:

1. Na początku wywołaj `beginCast()`. Metoda sprawdza `_caster`, gotowość
   umiejętności i uruchamia czas odnowienia.
2. Jeśli efekt ma powstać od razu, zwróć `std::make_shared<...>()`.
3. Jeśli efekt powstaje z opóźnieniem, zwróć `nullptr` i dodaj efekt później
   przez `_caster->addPendingSpawn(...)`.
4. Nie dodawaj encji bezpośrednio do `EntityManager`.

`PlayerController`, AI albo inny wywołujący odbiera wynik `cast(...)` i dopina
go do świata przez `spawnEntity(...)` albo `addPendingSpawn(...)`.

## 5. Pociski

Dla pocisków dziedzicz po `ProjectileAbility`, a nie bezpośrednio po `Ability`.
Ta klasa trzyma wspólne dane: nazwę pocisku, model, skalę, teksturę trafienia
i offset kierunku modelu.

Przykład minimalnej ability pociskowej:

```cpp
#pragma once

#include <ProjectileAbility.h>

namespace Nawia::Entity {

	class IceBoltAbility : public ProjectileAbility {
	public:
		IceBoltAbility(const std::string& model_path,
					   float model_scale,
					   const std::shared_ptr<Texture2D>& hit_tex,
					   const std::shared_ptr<Texture2D>& icon_tex)
			: ProjectileAbility(
				  "Ice Bolt",
				  "IceBolt",
				  AbilityTargetType::UNIT,
				  "Ice Bolt Projectile",
				  model_path,
				  model_scale,
				  hit_tex,
				  icon_tex) {}
	};

} // namespace Nawia::Entity
```

## 6. Przykład melee

Nagłówek:

```cpp
#pragma once

#include <Ability.h>

namespace Nawia::Entity {

	class SuperSlashAbility : public Ability {
	public:
		SuperSlashAbility(const std::shared_ptr<Texture2D>& slash_texture,
						  const std::shared_ptr<Texture2D>& icon_texture);

		AbilitySpawn cast(float target_x, float target_y) override;

	private:
		std::shared_ptr<Texture2D> _slash_texture;
	};

} // namespace Nawia::Entity
```

Implementacja:

```cpp
#include "SuperSlashAbility.h"

#include <Entity.h>
#include <SwordSlashEffect.h>

namespace Nawia::Entity {

	SuperSlashAbility::SuperSlashAbility(const std::shared_ptr<Texture2D>& slash_texture,
										 const std::shared_ptr<Texture2D>& icon_texture)
		: Ability("Super Slash", Entity::getAbilityStatsFromJson("SuperSlash"), AbilityTargetType::POINT, icon_texture),
		  _slash_texture(slash_texture) {}

	AbilitySpawn SuperSlashAbility::cast(const float target_x, const float target_y) {
		if (!beginCast())
			return nullptr;

		_caster->rotateTowardsCenter(target_x, target_y);
		_caster->playAnimation("attack", false, true);

		const Vector2 start_pos = _caster->getCenter();
		return std::make_shared<SwordSlashEffect>(
			start_pos.x,
			start_pos.y,
			-_caster->getRotation(),
			_slash_texture,
			_stats,
			_caster);
	}

} // namespace Nawia::Entity
```

## 7. Źródło użycia i pending spawns

Każda ability ma wskaźnik `_caster`. Ustawia go `Entity::addAbility(...)`.
Do sprawdzenia gotowości używaj `beginCast()` albo `canCast()`, zamiast ręcznie
sprawdzać `_caster` i `isReady()`.

Efekty nie powinny modyfikować listy aktywnych encji w trakcie iteracji po
świecie. Do tego służy bufor:

- natychmiastowy efekt zwraca `AbilitySpawn` z `cast(...)`,
- opóźniony efekt trafia do `_caster->addPendingSpawn(...)`,
- efekty wtórne, np. `ProjectileHitEffect`, mogą trafić do `addPendingSpawn(...)`
  encji, która je tworzy,
- `Engine` zbiera pending spawns po update i dopina je do świata.

## 8. Frakcje i filtrowanie

`Projectile` ignoruje źródło użycia, efekty umiejętności oraz cele z tej samej
frakcji. Efekty obszarowe powinny mieć własny filtr w `checkCollision(...)`,
jeśli mają działać tylko na wrogów, sojuszników albo konkretny typ encji.

## 9. Powiązane dokumenty

- `docs/guides/AbilityEffect_Guide.md`
- `docs/guides/Entity_Guide.md`
- `docs/guides/Enemy_Guide.md`
- `docs/guides/Coding_Standards.md`
