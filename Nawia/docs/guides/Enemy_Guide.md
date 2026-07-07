# Przewodnik po Enemy i Ally

Ten dokument opisuje jednostki bojowe: wrogow, sojusznikow, wspolny `ActorInterface` i aktualny model targetowania.

## Hierarchia

Jednostki bojowe opieraja sie o:

- `Entity` - baza wszystkich obiektow swiata,
- `ActorInterface` - wspolna baza actorow z mapa i targetem,
- `EnemyInterface` - specjalizacja wrogow,
- `AllyInterface` - specjalizacja sojusznikow,
- `AllyBrain` - opcjonalny obiekt decyzyjny dla ally.

`ActorInterface` przechowuje `_map` i `_target`, dzieki czemu enemy i ally nie duplikuja tej samej infrastruktury.

## Targetowanie

Cele sa odswiezane centralnie przez `EntityManager::refreshCombatTargets()`:

- enemy wybiera najblizszego `Player` albo `Ally`,
- ally wybiera najblizszego `Enemy`.

Typowa klasa enemy/ally nie musi recznie wyszukiwac celu. Powinna tylko reagowac na:

- brak celu,
- cel poza zasiegiem,
- cel w zasiegu ability,
- animacje ataku albo hit react.

Nietypowy priorytet celu mozna zrobic lokalnie w klasie albo w osobnym brainie,
jesli encja faktycznie go potrzebuje.

## Nowy enemy

Standardowy wzorzec:

1. Klasa dziedziczy po `EnemyInterface`.
2. Ma prywatny konstruktor.
3. Ma builder.
4. `update()` obsluguje stany.
5. Typ jest dodany w `EntityFactory`.

Szkielet:

```cpp
class Orc : public EnemyInterface {
public:
	void update(float dt) override;
	void takeDamage(int dmg) override;

private:
	Orc();
	friend class OrcBuilder;

	enum class State { Idle, Chasing, Attacking, GettingHit };
	State _state = State::Idle;
};
```

W konstruktorze ustaw zwykle:

- frakcje `Faction::Enemy`,
- model, animacje i skale,
- HP,
- opcjonalny collider,
- opcjonalne ability.

## Nowy ally

Ally dziala analogicznie, ale:

- ma `Faction::Ally`,
- walczy po stronie gracza,
- moze delegowac decyzje do `AllyBrain`.

Aktualny wzorzec to `Friend`, ktory ma `SwordSlashAbility` i proste zachowanie,
gdy nie ma podpietego braina.

## Czarownica

`Witch` to dystansowy enemy dla Czarownicy. Uzywa modelu
`assets/models/actors/witch/witch.glb` i indeksowanych animacji:

- `death`: 0,
- `get_hit`: 2,
- `idle`: 5,
- `bolt`: 7,
- `summon`: 10,
- `run`: 16.

Walka:

- stara sie utrzymac dystans od celu,
- strzela malym fireballem jako obecnym wariantem pocisku,
- po trafieniu odgrywa hit, a potem kontruje: powala gracza i przywoluje
  `WalkingDead`.

JSON spawnera:

```json
{
    "type": "witch",
    "name": "Czarownica",
    "hp": 160
}
```

## Typowy `update`

```cpp
void MyUnit::update(const float dt)
{
	if (isDying()) {
		Entity::update(dt);
		return;
	}

	if (isDormant())
		return;

	Entity::update(dt);
	updateAbilities(dt);

	if (!hasValidTarget()) {
		playAnimation("idle");
		return;
	}

	const float distance = getDistanceToTarget();
	const Vector2 target_pos = getTargetPosition();

	if (distance <= ATTACK_RANGE) {
		rotateTowardsCenter(target_pos.x, target_pos.y);
		// cast ability albo zmiana stanu
	} else {
		moveTo(target_pos.x, target_pos.y);
		updateMovement(dt);
	}
}
```

## `takeDamage`

Gdy jednostka ma reakcje na trafienie, nadpisz `takeDamage(...)`:

```cpp
void MyEnemy::takeDamage(const int damage)
{
	Entity::takeDamage(damage);
	if (isDying())
		return;

	_state = State::GettingHit;
	playAnimation("get_hit", false, true, 10, true);
	setVelocity(0, 0);
}
```

Najpierw zawsze wywolaj bazowe obrazenia, potem specjalna reakcje.

`Witch` jest wyjatkiem w sekwencji smierci: po animacji `death` zostaje
zamrozona na ostatniej klatce, zeby dialog po zwyciestwie pokazywal lezace
cialo. BossManager ukrywa encje dopiero po zamknieciu dialogu zwyciestwa.

## Ruch i mapy

`ActorInterface` daje dostep do `Map`, czyli mozna sprawdzac walkability i uzywac pathfindingu.

Do zwyklego chase czesto wystarcza:

- `moveTo(...)`,
- `updateMovement(dt)`,
- `chaseTarget(dt)`.

Do decyzji bojowych i projectile uzywaj `getCenter()`, a nie tylko `getX()/getY()`.

## JSON i factory

Jednostki tworzy `EntityFactory`, a dane przychodza z `assets/data/locations/objects_*.json`.

Przyklad:

```json
{
    "location": "Demo Arena",
    "type": "bandit",
    "name": "Bandyta",
    "x": 15.9,
    "y": 12.6,
    "hp": 80,
    "trigger_radius": 15.0,
    "abilities": ["KnifeThrow"]
}
```

Factory sklada obiekt i podpina assety. Zachowanie ma zostac w klasie aktora
albo brainie.

## Dobre praktyki

- Nie duplikuj wyszukiwania celu w kazdej klasie.
- Dla ally najpierw sprawdz, czy wystarczy proste lokalne zachowanie, a dopiero
  potem dodawaj brain.
- Collider jednostki nie zastepuje collidera efektu ataku.
- Stan animacji i stan zachowania trzymaj jawnie, gdy logika robi sie wieksza.
- Nie mieszaj respawnu, lokacji i zachowania aktora w jednej klasie.
