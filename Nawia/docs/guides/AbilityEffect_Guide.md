# Przewodnik po AbilityEffect

`AbilityEffect` to encja tworzona przez umiejetnosc: pocisk, cios obszarowy, efekt trafienia albo krotki obiekt bojowy obecny w swiecie.

## Odpowiedzialnosc

`AbilityEffect` rozszerza `Entity` o:

- `AbilityStats`,
- licznik czasu zycia,
- liste juz trafionych encji,
- bazowe sprawdzanie kolizji przez collider i bounding box.

Efekt wygasa, gdy `isExpired()` zwroci `true`. Usuwa go `EntityManager` podczas aktualizacji.

## Kolizje

Domyslny flow `checkCollision(target)`:

1. `canHitTarget(...)` odrzuca pusty cel, martwe/uspione encje, sam efekt i cele juz trafione.
2. Collider efektu jest porownywany z bounding boxem celu.
3. Jezeli szybki test przejdzie, collider robi dokladniejsze sprawdzenie z meshem celu.

Efekty specjalne moga to nadpisac. `Projectile` uzywa wlasnego 3D hitboxa, a `ProjectileHitEffect` zwraca `false`, bo jest tylko wizualem.

## Trafienie

`onCollision(target)` powinno:

1. sprawdzic, czy cel nadal istnieje,
2. policzyc obrazenia,
3. wywolac `target->takeDamage(...)`,
4. zapisac trafienie przez `addHit(target)`,
5. ewentualnie zakonczyc efekt przez `die()`.

Przyklad:

```cpp
void ShockWaveEffect::onCollision(const std::shared_ptr<Entity>& target) {
	if (!target)
		return;

	target->takeDamage(getDamage());
	addHit(target);
}
```

## Frakcje

Efekt powinien jawnie zdecydowac, kogo moze trafic:

- pociski ignoruja zrodlo uzycia i cele z tej samej frakcji,
- melee zwykle ignoruje castera,
- efekty wizualne ignoruja wszystko.

Nie zakladaj, ze `EntityManager` zrobi za efekt caly filtr friendly fire.

## Debug

Do strojenia hitboxow wlacz:

```cpp
Nawia::Entity::Entity::DebugColliders = true;
```

Wtedy widac collidery i pomocnicze bounding boxy. To jest szczegolnie przydatne przy `SwordSlashEffect`, projectile i triggerach.

## Dobre praktyki

- Najpierw sprawdz `canHitTarget(target)`.
- Po skutecznym trafieniu zawsze wolaj `addHit(target)`.
- Nie dodawaj nowych encji bezposrednio do `EntityManager`.
- Wizual bez obrazen powinien miec `checkCollision(...) == false`.
- Collider efektu stroimy pod gameplay, nie pod idealny wyglad modelu.
