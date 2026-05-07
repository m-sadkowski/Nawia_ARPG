# Przewodnik po Entity

Ten dokument opisuje aktualny model encji, ruchu, targetowania, colliderow i pending spawn w projekcie Nawia.

## Hierarchia

Najwazniejsze typy:

- `Entity` - baza wszystkich obiektow swiata,
- `ActorInterface` - baza jednostek bojowych z mapa i targetem,
- `EnemyInterface` - baza wrogow,
- `AllyInterface` - baza sojusznikow,
- `Player` - postac gracza,
- `AbilityEffect` - efekt umiejetnosci jako encja,
- `InteractiveClickable` i `InteractiveTrigger` - interakcje.

## Co daje `Entity`

Kazda encja ma:

- pozycje logiczna X/Y mapowana na X/Z swiata 3D,
- ruch przez `moveTo(...)` i `updateMovement(...)`,
- HP, smierc i sekwencje dying,
- model 3D i animacje,
- frakcje,
- targetowanie przez `setTarget(...)`, `hasValidTarget()`, `getTargetPosition()`,
- abilities i pending spawns,
- stan dormant.

Nowa encja rzadko powinna startowac od zera. Dla jednostek bojowych uzywaj `EnemyInterface` albo `AllyInterface`.

## Pozycja i wysokosc

Najwazniejsze helpery:

- `getX()` / `getY()` - pozycja logiczna na mapie,
- `getWorldPosition()` - pozycja w 3D,
- `getCenter()` - srodek modelu, najlepszy do celowania,
- `rotateTowards(...)` - obrot do punktu z pozycji logicznej,
- `rotateTowardsCenter(...)` - obrot do punktu z centrum encji.

Do ruchu wystarczy zwykle `getX()/getY()`. Do walki i projectile uzywaj `getCenter()`.

## Collider

Nie kazda encja potrzebuje collidera.

Collider jest wazny dla:

- efektow ability,
- triggerow,
- klikalnych obiektow wymagajacych precyzji,
- specjalnych mechanik.

Typy:

- `CircleCollider` - pociski i radialne efekty,
- `RectangleCollider` - trigger, skrzynia, prosty obiekt,
- `ConeCollider` - melee w stylu `SwordSlash`.

Zwykle jednostki bojowe sa odpychane centralnie przez `EntityManager`, a collider efektu odpowiada za trafienie ataku.

## Hover, klik i mesh

Klikanie encji dziala przez raycast:

1. szybki test bounding box,
2. dokladniejszy test mesha, jesli model istnieje,
3. fallback box dla encji bez modelu.

Dla modeli trudnych do trafienia zwieksz fallback/interaction range albo popraw collider/mesh, zamiast dodawac sztuczne UI hacki.

## `update`

Typowy pattern:

```cpp
void MyEntity::update(const float dt)
{
	if (isDying()) {
		Entity::update(dt);
		return;
	}

	if (isDormant())
		return;

	Entity::update(dt);
	updateAbilities(dt);

	// logika wlasna
}
```

`Entity::update(dt)` obsluguje bazowy ruch, animacje i koniec sekwencji smierci.

## Obrazenia

`Entity::takeDamage(...)`:

- odejmuje HP,
- uruchamia dying, gdy HP spadnie do zera,
- zmienia frakcje martwej encji na `None`,
- zostawia encje przy zyciu do zakonczenia animacji smierci.

Nadpisuj `takeDamage(...)`, gdy encja ma hit react, stagger albo specjalna logike.

## Dormant i lokacje

Dormant encja:

- nie renderuje sie,
- nie aktualizuje AI,
- nie bierze udzialu w kolizjach,
- nie ma healthbara,
- nie reaguje na hover/klik.

Tego uzywa `SpawnManager` przy lokacjach i proximity spawnach.

## Pending spawns

Encja moze dodac nowa encje przez:

```cpp
addPendingSpawn(effect);
```

`Engine` zbiera pending spawns po update i dopina je do swiata. To chroni `EntityManager` przed modyfikacja listy encji podczas iteracji.

## Dodanie nowej encji do JSON

1. Dodaj klase encji.
2. Dodaj builder, jesli pasuje do istniejacego wzorca.
3. Dodaj obsluge typu w `EntityFactory`.
4. Dodaj wpis w `assets/data/level_entities/<level>.json`.
5. Sprawdz, czy encja dostaje mape, frakcje, model i HP.

## Dobre praktyki

- Factory sklada obiekt, nie prowadzi AI.
- Do targetowania w walce uzywaj helperow z `Entity`.
- Nie rob bezposredniego dostepu do `EntityManager` z ability.
- Debug colliderow wlaczaj przez `Entity::DebugColliders`.
- Stan uzywany tylko przez subclass trzymaj w subclass.
