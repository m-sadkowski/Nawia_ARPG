# Przewodnik po Entity

Ten dokument opisuje aktualny sposob tworzenia encji w projekcie Nawia oraz miejsca, w ktorych podpina sie logike walki, AI i spawning.

## 1. Hierarchia klas

Podstawowa hierarchia wyglada teraz tak:

- `Entity` - baza dla wszystkich obiektow swiata
- `ActorInterface` - wspolna baza jednostek bojowych z mapa i targetem
- `EnemyInterface` - specjalizacja dla wszystkich wrogow
- `AllyInterface` - specjalizacja dla wszystkich sojusznikow
- `Player` - sterowana przez gracza postac
- `AbilityEffect` - efekt umiejetnosci istniejący jako encja w swiecie

Najwazniejsze pliki:

- `src/entity/Entity.h`
- `src/entity/actors/ActorInterface.h`
- `src/entity/actors/enemies/EnemyInterface.h`
- `src/entity/actors/allies/AllyInterface.h`
- `src/world/spawn/EntityFactory.cpp`

## 2. Co daje klasa Entity

Kazda `Entity` ma juz gotowe:

- pozycje i ruch (`moveTo`, `updateMovement`)
- HP i obrazenia (`takeDamage`, `die`, `isDead`)
- model 3D i animacje (`loadModel`, `addAnimation`, `playAnimation`)
- frakcje (`Faction::Player`, `Enemy`, `Ally`, `Neutral`, `None`)
- targetowanie (`setTarget`, `hasValidTarget`, `getDistanceToTarget`, `chaseTarget`)
- umiejetnosci (`addAbility`, `getAbility`, `updateAbilities`)
- obsluge dormant (`setDormant`, `isDormant`)

W praktyce oznacza to, ze nowa encja bardzo rzadko zaczyna "od zera". Najczesciej dziedziczymy po `EnemyInterface` albo `AllyInterface`, a nie bezposrednio po `Entity`. Obie klasy korzystaja z `ActorInterface`, wiec mapa i targetowanie sa wspolne dla enemy oraz ally.

## 3. Kiedy dziedziczyc po czym

Uzyj `Entity`, gdy tworzysz:

- prosty obiekt swiata
- trigger
- efekt umiejetnosci
- cos, co nie ma typowego AI combatowego

Uzyj `EnemyInterface`, gdy tworzysz:

- jednostke wroga
- encje, ktora ma byc celem dla ally i gracza
- postac korzystajaca z mapy do ruchu

Uzyj `AllyInterface`, gdy tworzysz:

- jednostke pomagajaca graczowi
- postac, ktora ma atakowac wrogow
- obiekt, do ktorego w przyszlosci chcesz podpiac `AllyBrain`

## 4. Minimalna implementacja nowej encji

Przyklad dla zwyklej encji:

```cpp
#pragma once

#include "Entity.h"

namespace Nawia::Entity {

	class TrainingStatue : public Entity {
	public:
		void update(float dt) override;

	private:
		friend class TrainingStatueBuilder;
		TrainingStatue();
	};

}
```

```cpp
#include "TrainingStatue.h"

namespace Nawia::Entity {

	TrainingStatue::TrainingStatue()
		: Entity("Training Statue", 0.0f, 0.0f, nullptr, 250)
	{
		setFaction(Faction::None);
		setScale(0.02f);

		loadModel("../assets/models/statue_idle.glb");
		addAnimation("idle", "../assets/models/statue_idle.glb");
		playAnimation("idle");
	}

	void TrainingStatue::update(const float dt)
	{
		Entity::update(dt);
	}

}
```

Jesli encja ma byc spawniona przez JSON, trzeba jeszcze dodac ja do `EntityFactory`.

## 5. Pozycjonowanie, rotacja i targetowanie

Najwazniejsze metody przestrzenne:

- `getX()` / `getY()` - pozycja logiczna encji na plaszczyznie mapy
- `getCenter()` - srodek encji, najczesciej najlepszy do walki i celowania
- `rotateTowards(x, y)` - obrot w strone punktu liczony od pozycji encji
- `rotateTowardsCenter(x, y)` - obrot do celu liczony pod katem celowania w srodek

Praktyczna zasada:

- do ruchu AI i pathowania zazwyczaj wystarczy `getX()/getY()`
- do walki, pociskow i atakow melee prawie zawsze uzywaj `getCenter()`

Przyklad:

```cpp
const Vector2 target_pos = target->getCenter();
rotateTowardsCenter(target_pos.x, target_pos.y);
```

## 6. Collidery - kiedy i po co ich uzywac

Nie kazda encja musi miec collider.

W obecnym projekcie:

- fizyczne odpychanie actorow (`Player`, `Enemy`, `Ally`) jest rozwiazywane centralnie przez `EntityManager::resolveOverlap()`
- collidery nadal sa bardzo wazne dla:
  - `AbilityEffect`
  - triggerow
  - wybranych encji, ktore potrzebuja precyzyjniejszego obszaru

Najczestsze typy:

- `RectangleCollider` - dobre dla triggerow i prostokatnych obszarow
- `CircleCollider` - dobre dla pociskow i radialnych efektow
- `ConeCollider` - dobre dla atakow w stylu `SwordSlash`

Przyklad podpiecia:

```cpp
setCollider(std::make_unique<RectangleCollider>(this, 1.0f, 1.2f, 0.0f, 0.0f));
```

Wazna praktyka:

- dla zwyklych jednostek bojowych collider jest opcjonalny
- dla efektow umiejetnosci collider zwykle jest glownym nosem logiki trafienia
- nie probuj przenosic calej fizyki postaci do colliderow, bo ta warstwa jest juz rozdzielona

## 7. Broadphase, mesh i trafienia

Projekt korzysta z dwoch poziomow sprawdzania trafienia:

1. broadphase, zwykle przez obreb `BoundingBox`
2. dokladniejsze sprawdzenie kolizji lub ray / mesh hit w zaleznosci od typu obiektu

To dlatego encje i efekty potrafia korzystac jednoczesnie z:

- `getBoundingBox()`
- colliderow
- funkcji z `AbilityEffect::checkCollision(...)`

Przy implementacji nowego efektu najczestszy pattern wyglada tak:

```cpp
bool MyEffect::checkCollision(const std::shared_ptr<Entity>& target) const
{
	if (target->isDead()) return false;
	return AbilityEffect::checkCollision(target);
}
```

## 8. Jak debugowac collidery i hitboxy

Do debugowania geometrii trafien i colliderow uzywaj:

```cpp
Nawia::Entity::Entity::DebugColliders = true;
```

Przy wlaczonym debug:

- renderowane sa collidery encji i efektow, jesli je maja
- widac tez pomocnicze bounding boxy modeli

To jest bardzo przydatne przy:

- strojeniu zasiegu melee
- ustawianiu triggerow portal/checkpoint
- sprawdzaniu, czy efekt trafia tam, gdzie myslisz

## 9. Logika encji w `update()`

Bazowy pattern jest prosty:

1. obsluz przypadek `isDying()`
2. obsluz przypadek `isDormant()`
3. wywolaj `Entity::update(dt)` jesli chcesz bazowego ruchu/animacji
4. dopiero potem rob logike wlasna

Przyklad:

```cpp
void MyEntity::update(const float dt)
{
	if (isDying())
	{
		Entity::update(dt);
		return;
	}

	if (isDormant()) return;

	Entity::update(dt);
	updateAbilities(dt);

	// logika wlasna
}
```

Jesli jednostka ma stany, najlepiej trzymac je jawnie:

```cpp
enum class State { Idle, Chasing, Attacking, GettingHit };
```

## 10. `takeDamage()` i reakcja na hit

Domyslna implementacja `Entity::takeDamage()`:

- odejmuje HP
- przy smiertelnym ciosie odpala sekwencje dying
- ustawia frakcje na `Faction::None`

Jesli encja ma reagowac specjalnie na hit, nadpisz `takeDamage()` i po wywolaniu `Entity::takeDamage(dmg)` ustaw:

- stan `GettingHit`
- odpowiednia animacje
- ewentualnie zatrzymanie ruchu

To jest aktualny pattern np. w kilku enemy.

## 11. Jak tworzyc nowego enemy

Najwygodniejszy wzorzec w repo to:

1. nowa klasa dziedziczy po `EnemyInterface`
2. ma prywatny konstruktor i `Builder`
3. `update()` steruje stanami
4. `EntityFactory` zna nowy typ JSON

Szkielet:

```cpp
class MyEnemy : public EnemyInterface {
public:
	void update(float dt) override;

private:
	MyEnemy();
	friend class MyEnemyBuilder;
};
```

W konstruktorze zwykle:

- ustawiasz `setFaction(Faction::Enemy)`
- ladujesz model i animacje
- ustawiasz `setScale(...)`
- opcjonalnie collider
- opcjonalnie zdolnosci przez `addAbility(...)`

Targetowanie enemy jest teraz odswiezane centralnie w `EntityManager::refreshCombatTargets()`. Standardowy wrog szuka najblizszego:

- `Player`
- albo `Ally`

Nie trzeba juz recznie wyszukiwac targetu w kazdej klasie enemy, chyba ze chcesz zrobic niestandardowe zachowanie.

## 12. Jak tworzyc nowego ally

Sojusznicy dzialaja analogicznie do enemy, tylko opieraja sie o `AllyInterface`.

Aktualny przyklad referencyjny:

- `src/entity/actors/allies/friend/Friend.h`
- `src/entity/actors/allies/friend/Friend.cpp`

`Friend` ma obecnie:

- frakcje `Faction::Ally`
- 100 HP
- `SwordSlashAbility`
- hardcoded zachowanie: atakuj najblizszego wroga

Minimalny szkielet ally:

```cpp
class MyAlly : public AllyInterface {
public:
	void update(float dt) override;

private:
	MyAlly();
	friend class MyAllyBuilder;
};
```

W konstruktorze ally zwykle:

- ustawiasz `setFaction(Faction::Ally)`
- ustawiasz HP
- ladujesz model i animacje
- dodajesz ability
- opcjonalnie podpinasz brain

Targetowanie ally jest tez odswiezane centralnie przez `EntityManager`. Domyslnie ally szuka najblizszego `Enemy`.

## 13. AllyBrain - jak dziala teraz

`AllyBrain` jest teraz szkieletem pod przyszly system ET.

Pliki:

- `src/entity/actors/allies/AllyBrain.h`
- `src/entity/actors/allies/AllyBrain.cpp`

Interfejs jest celowo prosty:

```cpp
class AllyBrain {
public:
	virtual ~AllyBrain() = default;
	virtual void update(AllyInterface& ally, float dt);
};
```

`AllyInterface` trzyma wskaznik:

- `setBrain(const std::shared_ptr<AllyBrain>& brain)`
- `getBrain()`

Obecna zasada jest taka:

- jesli ally ma brain, `update()` moze delegowac zachowanie do braina
- jesli braina nie ma, encja moze uzyc hardcoded fallbacku

Tak wlasnie dziala teraz `Friend`.

## 14. Jak w przyszlosci dodac nowy AllyBrain

Przyklad:

```cpp
#pragma once

#include "AllyBrain.h"

namespace Nawia::Entity {

	class GuardBrain : public AllyBrain {
	public:
		void update(AllyInterface& ally, float dt) override;
	};

}
```

```cpp
#include "GuardBrain.h"

#include "AllyInterface.h"

namespace Nawia::Entity {

	void GuardBrain::update(AllyInterface& ally, const float dt)
	{
		(void)dt;

		if (!ally.hasValidTarget())
			return;

		const Vector2 target_pos = ally.getTargetPosition();
		ally.moveTo(target_pos.x, target_pos.y);
		ally.updateMovement(dt);
	}

}
```

Potem w konstruktorze lub factory:

```cpp
auto ally = MyAllyBuilder()
	.setName(name)
	.setPosition({x, y})
	.setMap(map)
	.setMaxHp(120)
	.setBrain(std::make_shared<GuardBrain>())
	.build();
```

Dobra praktyka na przyszlosc:

- logike decyzji trzymaj w brainie
- konfiguracje encji trzymaj w klasie ally
- spawning i dobieranie assetow trzymaj w `EntityFactory`

## 15. Jak podpiac encje do spawnow JSON

Kazdy nowy typ encji, ktory ma byc tworzone z pliku levelu, musi przejsc przez `EntityFactory`.

Kroki:

1. Dodaj include nowej klasy w `src/world/spawn/EntityFactory.cpp`
2. Dodaj obsluge typu w `EntityFactory::create(...)`
3. Dodaj prywatna metode typu `createMyAlly(...)` albo `createMyEnemy(...)`
4. Jesli potrzeba, dodaj ability albo brain w factory

Przyklad wpisu w JSON:

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

## 16. Dormant i lokacje

System lokacji nie usuwa encji przy kazdym teleporcie. Zamiast tego encje sa:

- aktywne
- albo dormant

Encja dormant:

- nie renderuje sie
- nie aktualizuje AI
- nie bierze udzialu w kolizjach
- nie ma world-space healthbara

To jest glowny mechanizm przelaczania encji miedzy lokacjami.

## 17. Umiejetnosci i pending spawns

Jesli encja uzywa umiejetnosci, zwykle nie dodaje efektu bezposrednio do `EntityManager`.
Zamiast tego:

- ability tworzy efekt
- encja wrzuca go przez `addPendingSpawn(...)`
- `Engine` zbiera pending spawns po update i dodaje je do swiata

To jest wazne, bo tak dziala teraz m.in. `SwordSlashAbility` i rzuty pociskow.

## 18. Dobre praktyki

- W `update()` zawsze pilnuj `isDying()` i `isDormant()`.
- Do ruchu i targetowania korzystaj z helperow z `Entity`, jesli nie potrzebujesz specjalnego zachowania.
- Do walki uzywaj `getCenter()`, nie surowego `getX()/getY()`, jesli liczy sie celowanie w srodek celu.
- Dla efektow obszarowych najpierw mysl o colliderze, dopiero potem o wizualu.
- Builder trzymaj obok klasy, jesli encja ma byc tworzona podobnie jak reszta actorow.
- Nie mieszaj logiki decision making z factory. Factory ma skladac obiekt, nie prowadzic AI.

## 19. Powiazane dokumenty

- `docs/guides/Enemy_Guide.md`
- `docs/guides/Ability_Guide.md`
- `docs/guides/Level_Guide.md`
- `docs/guides/Coding_Standards.md`
- `src/entity/README.md`
