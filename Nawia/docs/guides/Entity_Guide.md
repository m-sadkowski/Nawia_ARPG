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
4. Dodaj wpis w `assets/data/locations/objects_<lokacja>.json` albo ustaw encje w Kreatorze leveli.
5. Sprawdz, czy encja dostaje mape, frakcje, model i HP.

## Fabularni NPC

Fabularne NPC dziedzicza zwykle po `StoryNpc`.

- `GenericStoryNpc` obsluguje pojedynczych ludzi i zielarza konfigurowanych z JSON-a przez `model`, `animation_bundle`, nazwy animacji, `dialogue_key` oraz opcjonalna trase po dialogu.
- `CemeterySurvivorGroupNpc` jest zlozona encja dla dwojga ocalonych z cmentarza. Encja glowna to `female_warrior`, a dodatkowy wizual to `male_npc_1`; jeden dialog wysyla oboje do huba zielarza i dopiero dotarcie grupy zalicza checkpoint.
- `ForestLostGroupNpc` jest zlozona encja dla grupy z lasu. Encja glowna to `female_npc_2`, a dodatkowe wizuale to `male_npc_2` i `milena_sister`.

`StoryNpc` obraca sie w strone pobliskiego gracza, jezeli w danej chwili
`canInteract()` zwraca true i NPC nie idzie. Dzieki temu zachowanie dotyczy nie
tylko Mushrooma, ale tez ludzi fabularnych i grup NPC. Wyjatkiem sa obiekty,
ktore nadpisza `shouldLookAtPlayerWhenNearby()`, np. `WandaCorpseNpc`.

`CemeterySurvivorGroupNpc` uzywa tych samych indeksow animacji co pozostali
tymczasowi NPC: `9` dla idle i `16` dla chodu. Po dotarciu do `Herbalist Hub`
ocaleni wybieraja losowe punkty w promieniu huba i dochodza tam ruchem.

`ForestLostGroupNpc` ma kilka etapow: oczekiwanie na rozmowe, niesienie siostry
Mileny do `Herbalist Hub`, opuszczenie jej na ziemie, wstawanie z animacji
`Death` od konca i rozejscie sie calej trojki do losowych punktow w hubie.
Parametry niesienia trzymane sa w `CarryTuning`, a indeksy animacji w
`AnimationIndices`, zeby strojenie JSON-em nie rozlewalo sie po logice stanu.

`Entity::loadModel` uzupelnia brakujace bufory animacji dla skinned modeli,
ktore maja dodatkowe nieskinned meshe, np. miecz w `female_warrior.glb`.
Bez tego jeden akcesoryjny mesh potrafil blokowac `UpdateModelAnimation` dla
calej postaci i zostawial ja w T-pose.

`female_warrior.glb` ma osobny mesh `1` dla miecza. Encje fabularne uzywajace
tego modelu wywoluja `hideMeshIndex(1)`, zeby ocalona z cmentarza nie nosila
broni.

## Dobre praktyki

- Factory sklada obiekt, nie prowadzi AI.
- Do targetowania w walce uzywaj helperow z `Entity`.
- Nie rob bezposredniego dostepu do `EntityManager` z ability.
- Debug colliderow wlaczaj przez `Entity::DebugColliders`.
- Stan uzywany tylko przez subclass trzymaj w subclass.
