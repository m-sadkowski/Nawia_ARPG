# Przewodnik po AbilityEffect

`AbilityEffect` (`src/entity/abilities/AbilityEffect.h`) to encja tworzona przez
umiejętność: pocisk, stożek cięcia, krótkotrwały efekt trafienia albo inny obiekt
bojowy obecny w świecie gry.

## Odpowiedzialność

`AbilityEffect` rozszerza `Entity` o:

- `AbilityStats`, czyli obrażenia, czas życia i rozmiar trafienia,
- licznik czasu życia `_timer`,
- listę trafionych encji, żeby jeden efekt nie zadawał obrażeń wielokrotnie,
- bazowy dwufazowy test kolizji dla efektów z koliderem.

Efekt wygasa, gdy `isExpired()` zwróci `true`. Usuwa go wtedy `EntityManager`
podczas aktualizacji encji.

## Kolizje

Bazowe `checkCollision(target)` działa tak:

1. `canHitTarget(...)` odrzuca pusty cel, martwe/uśpione encje, sam efekt oraz
   encje już trafione.
2. Kolider efektu jest porównywany z pudełkiem ograniczającym celu.
3. Jeśli szybki test przejdzie, kolider wykonuje dokładniejszy test z siatką
   modelu celu.

Efekty specjalne mogą nadpisać ten przebieg. `Projectile` używa własnego pudełka
trafienia 3D, a `ProjectileHitEffect` całkowicie wyłącza kolizję, bo jest tylko
efektem wizualnym.

## Trafienie

`onCollision(target)` jest wywoływane przez `EntityManager`, gdy `checkCollision`
zwróci `true`. Implementacja efektu powinna:

1. policzyć finalne obrażenia,
2. wywołać `target->takeDamage(...)` albo inną reakcję,
3. zapisać trafienie przez `addHit(target)`,
4. opcjonalnie zakończyć życie efektu przez `die()`.

Przykład:

```cpp
void ShockWaveEffect::onCollision(const std::shared_ptr<Entity>& target) {
	if (!target)
		return;

	target->takeDamage(getDamage());
	addHit(target);
}
```

## Efekty wtórne

Efekt może tworzyć kolejne encje przez własny bufor pending spawnów. Tak działa
`Projectile`: po trafieniu dodaje `ProjectileHitEffect` przez `addPendingSpawn(...)`,
a `Engine` dopina ten efekt do świata po zakończeniu iteracji.

## Dobre praktyki

- W `checkCollision(...)` najpierw używaj `canHitTarget(target)`.
- Nie zakładaj, że `target` istnieje; sprawdź pusty `shared_ptr`.
- Dodawaj `addHit(target)` po skutecznym trafieniu.
- Nie dodawaj efektów bezpośrednio do `EntityManager`.
- Dla efektów wizualnych bez obrażeń nadpisz `checkCollision(...)` i zwróć `false`.
