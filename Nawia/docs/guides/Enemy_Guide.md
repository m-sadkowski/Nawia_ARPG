# Przewodnik po Enemy i Ally

Ten dokument opisuje aktualny przeplyw tworzenia jednostek combatowych:

- `EnemyInterface`
- `AllyInterface`
- konkretne klasy jak `Bandit`, `WalkingDead`, `Devil`, `Friend`

## 1. Wspolny model dla jednostek bojowych

Jednostki bojowe w projekcie opieraja sie o:

- `Entity` - wspolna baza
- `ActorInterface` - wspolna baza dla actorow majacych mape i target
- `EnemyInterface` - specjalizacja wrogow
- `AllyInterface` - specjalizacja sojusznikow

`ActorInterface` daje:

- `_map` do ruchu po levelu
- `_target` do sledzenia celu
- builder z `setMap(...)` i `setTarget(...)`

`EnemyInterface` ustawia typ encji na `EntityType::Enemy`. `AllyInterface` ustawia typ encji na `EntityType::Ally` i ma dodatkowo:

- `setBrain(...)`
- `getBrain()`

Dzieki temu targetowanie oraz wskaznik mapy nie sa juz duplikowane osobno w enemy i ally.

## 2. Jak tworzyc nowego enemy

Najczestszy wzorzec w repo:

1. klasa dziedziczy po `EnemyInterface`
2. ma prywatny konstruktor
3. ma `Builder`
4. `update()` obsluguje stany

Przyklad:

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

W konstruktorze enemy zwykle:

- `setFaction(Faction::Enemy)`
- `setScale(...)`
- `loadModel(...)`
- `addAnimation(...)`
- `playAnimation("default")` albo `playAnimation("idle")`
- opcjonalnie `setCollider(...)`
- opcjonalnie `addAbility(...)`

## 3. Jak tworzyc nowego ally

Ally tworzy sie prawie identycznie jak enemy.

Roznica polega glownie na tym, ze:

- ma `Faction::Ally`
- walczy po stronie gracza
- moze miec `AllyBrain`

Przyklad aktualny:

- `src/entity/actors/allies/friend/Friend.h`
- `src/entity/actors/allies/friend/Friend.cpp`

`Friend` jest teraz referencyjnym prostym ally:

- ma 100 HP
- ma `SwordSlashAbility`
- bez braina uzywa hardcoded zachowania
- atakuje najblizszego wroga

## 4. Targetowanie - co dzieje sie automatycznie

Od teraz targetowanie jest odswiezane centralnie w:

- `src/core/game/EntityManager.cpp`

Metoda `refreshCombatTargets()` ustawia cele co tick:

- enemy -> najblizszy `Player` albo `Ally`
- ally -> najblizszy `Enemy`

To oznacza, ze w typowej klasie enemy/ally nie trzeba juz robic recznego wyszukiwania targetu. Klasa jednostki moze skupic sie na pytaniach:

- czy isc do celu
- czy castowac
- czy grac animacje ataku
- co zrobic po utracie targetu

Jesli chcesz nietypowy priorytet celu, wtedy masz dwie opcje:

1. nadpisac target lokalnie w swojej klasie
2. zrobic w przyszlosci bardziej zaawansowany brain

## 5. Przykladowy flow `update()`

Typowy pattern dla combat entity:

```cpp
void MyUnit::update(const float dt)
{
	if (isDying())
	{
		Entity::update(dt);
		return;
	}

	if (isDormant()) return;

	Entity::update(dt);
	updateAbilities(dt);

	if (!hasValidTarget())
	{
		playAnimation("idle");
		return;
	}

	const float dist = getDistanceToTarget();
	const Vector2 target_pos = getTargetPosition();

	if (dist <= 1.5f)
	{
		rotateTowardsCenter(target_pos.x, target_pos.y);
		// użycie umiejętności, cios wręcz albo zmiana stanu
	}
	else
	{
		moveTo(target_pos.x, target_pos.y);
		updateMovement(dt);
	}
}
```

## 6. `takeDamage()`, hit react i animacje

W jednostkach bojowych bardzo czesto warto nadpisac `takeDamage(int dmg)`.

Typowy pattern:

```cpp
void MyEnemy::takeDamage(const int dmg)
{
	Entity::takeDamage(dmg);
	if (isDying()) return;

	_state = State::GettingHit;
	playAnimation("get_hit", false, true, 10, true);
	setVelocity(0, 0);
}
```

To daje trzy rzeczy:

- encja realnie traci HP
- po smiertelnym ciosie przechodzi do dying i nie robi juz nic wiecej
- po zwyklym ciosie mozna odpalic stagger / get hit / przerwanie ataku

W praktyce warto tez trzymac:

- `_state_before_hit`
- powrot do poprzedniego stanu po zakonczeniu animacji

## 7. Map, walkability i ruch

`EnemyInterface` i `AllyInterface` dziedzicza dostep do `_map` z `ActorInterface`.

To przydaje sie do:

- sprawdzania `isWalkable(...)`
- wybierania punktu retreat / chase
- blokowania dasha przez sciane

Przyklad:

```cpp
if (_map && !_map->isWalkable(next_x, next_y))
{
	// zmien stan albo przerwij ruch
}
```

Jesli nie potrzebujesz specjalnego pathowania, w wielu przypadkach wystarczy:

- `moveTo(...)`
- `updateMovement(dt)`

## 8. Pushing i fizyka actorow

Jednostki bojowe sa odpychane automatycznie przez `EntityManager`.

To oznacza, ze:

- `Player`
- `Enemy`
- `Ally`

nie powinny stac idealnie w tym samym miejscu.

Zwykle nie trzeba pisac do tego zadnej dodatkowej logiki w klasie jednostki. Wyjatek to przypadki specjalne, jak dash albo teleport pozycji.

## 9. Ability w jednostkach bojowych

Jednostki bojowe dodaja skille przez:

```cpp
addAbility(std::make_shared<MyAbility>(...));
```

Potem w `update()`:

- pobierasz ability przez `getAbility(index)`
- sprawdzasz `isReady()`
- uruchamiasz `cast(...)`

Jesli ability tworzy efekt z opoznieniem albo przez animacje, zwykle sama wrzuci wynik do `pending spawns`.

Przyklad obecny:

- `SwordSlashAbility`
- `KnifeThrowAbility`

## 10. Kiedy dawac collider jednostce

Wiele jednostek bojowych moze dzialac bez rozbudowanego collidera, ale collider nadal bywa przydatny, gdy:

- chcesz miec lepszy pivot dla celu
- dana encja jest triggerowana lub zderzana niestandardowo
- specjalna mechanika potrzebuje konkretnego ksztaltu

Przyklad:

```cpp
setCollider(std::make_unique<RectangleCollider>(this, 1.f, 1.2f, 0.0f, 0.0f));
```

Najwazniejsze: nie myl collidera encji z colliderem efektu ataku. Melee i projectile zwykle powinny miec swoja wlasna geometrie w `AbilityEffect`.

## 11. Spawning jednostek z JSON

Jednostki bojowe powinny byc tworzone przez `EntityFactory`, a nie recznie w levelu.

Aktualna sciezka:

1. wpis w `assets/data/level_entities/...json`
2. `SpawnManager` wczytuje definicje
3. `EntityFactory::create(...)` buduje obiekt
4. encja trafia do `EntityManager`

Przyklad enemy:

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

Przyklad ally:

```json
{
    "location": "Demo Arena",
    "type": "friend",
    "name": "Friend",
    "x": 9.0,
    "y": 24.0,
    "hp": 100,
    "trigger_radius": 0
}
```

## 12. Dormant i lokacje

Jednostki nie sa niszczone przy teleportach miedzy lokacjami. Zamiast tego sa zamrazane przez `setDormant(true)`.

Wazne skutki:

- dormant unit nie renderuje modelu
- dormant unit nie aktualizuje AI
- dormant unit nie ma healthbara w HUD
- dormant unit nie bierze udzialu w kolizjach

SpawnManager pamieta tez, czy jednostka byla juz aktywowana przed opuszczeniem lokacji. Dzieki temu po powrocie wraca do poprzedniego stanu aktywacji zamiast czekac drugi raz na proximity trigger.

## 13. Jak w przyszlosci dodac AllyBrain

`AllyBrain` ma byc miejscem na logike decyzyjna dla sojusznikow.

Zalecany podzial odpowiedzialnosci:

- klasa `MyAlly`:
  - assety
  - HP
  - abilities
  - animacje
  - fallback behavior
- klasa `MyAllyBrain`:
  - decyzje
  - priorytety celu
  - wybieranie skilla
  - ET / BT / FSM

Minimalny przyklad braina:

```cpp
class SupportBrain : public AllyBrain {
public:
	/** @brief Aktualizuje decyzje sojusznika w danej klatce. */
	void update(AllyInterface& ally, float dt) override;
};
```

Podpiecie:

```cpp
auto ally = MyAllyBuilder()
	.setName(name)
	.setPosition({x, y})
	.setMap(map)
	.setMaxHp(100)
	.setBrain(std::make_shared<SupportBrain>())
	.build();
```

W samym `update()` ally najlepiej zostawic taki pattern:

- jesli `getBrain()` zwraca obiekt -> oddaj sterowanie brainowi
- w przeciwnym razie uzyj zachowania hardcoded

To pozwala wdrazac nowe brainy stopniowo, bez przepisywania kazdej klasy ally od razu.

## 14. Checklist przy dodawaniu nowej jednostki

1. Dodaj klase `.h/.cpp`
2. Ustaw frakcje, HP, model i animacje
3. Zdecyduj, czy potrzeba collidera encji
4. Dodaj `takeDamage()` / hit react, jesli jednostka ma reagowac na trafienie
5. Dodaj ability, jesli potrzebne
6. Dodaj builder
7. Zarejestruj typ w `EntityFactory`
8. Dodaj wpis w JSON levelu
9. Jesli to ally, zdecyduj:
   hardcoded fallback czy `AllyBrain`

## 15. Powiazane pliki

- `docs/guides/Entity_Guide.md`
- `docs/guides/Ability_Guide.md`
- `src/entity/actors/allies/AllyBrain.h`
- `src/world/spawn/EntityFactory.cpp`
