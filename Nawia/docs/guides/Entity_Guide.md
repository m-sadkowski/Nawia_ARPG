# Przewodnik po klasie Entity

Klasa `Entity` (`src/entity/Entity.h`) jest bazową klasą dla absolutnie wszystkich obiektów wizualnych w grze Nawia ARPG. Reprezentuje ona każdy byt, który funkcjonuje przestrzennie, korzysta z zasobów 3D (`loadModel`), posiada punkty życia i ewentualnie system inteligencji oraz reaguje na oświetlenie lub ray-collision na silniku.

## Czym jest Entity?
To scentralizowany port na całą abstrakcję postaci lub bryły:
- **Transformacja**: Pozycja 3D rzutowana wektorową mapą podłogi (`_pos.x`, `_pos.y` jako Oś Głębokości), wbudowany ułamek skali, rotacja.
- **Grafika**: Moduły zaczytywania modeli .glb wraz ze stosem zaimplementowanego mechanizmu animacyjnego 60 klatek na sekundę.
- **Fizyka**: Metody natywnej ewaluacji pudła `BoundingBox` generującego brzegowe krawędzie z uwzględnieniem animacji dla testów Broadphase.
- **Logika gry**: Paski odporności, punkty HP, przynależności frakcyjne (`_faction`).

## Jak zaimplementować własny obiekt?

Stworzenie nowego potwora czy NPC zazwyczaj objawia się dziedziczeniem publicznym po `Nawia::Entity::Entity` lub po pobranym do systemu wrogu `EnemyInterface`.

### Krok 1: Implementacja (.cpp)
```cpp
#include "MyCustomNPC.h"

namespace Nawia::Entity {

    MyCustomNPC::MyCustomNPC(float x, float y)
        : Entity("Shopkeeper", x, y, nullptr, 100) 
    {
        // ------------------------------------------------------------------
        // WERSJA ARCHITEKTURY - GRAFIKA 3D
        // ------------------------------------------------------------------
        // Obecny silnik jest przystosowany pod Raylib 3D Models. Od razu wrzucamy:
        loadModel("../assets/models/shopkeeper.glb");
        
        // Dodawanie pętli akcji
        addAnimation("idle", "../assets/models/shopkeeper_idle.glb");
        playAnimation("idle");

        // Skalowanie naturalne w silniku (przy nieadekwatności eksportów bywa przydatne)
        setScale(0.04f);

        // WAŻNE: Całe odpychanie modelu względem ścian fizycznych zajmie się już EntityManager
        // opartym na radiusowej matematyce! Nie musisz mu wręczać starych obiektów RectangleCollider!
        
        // Frakcja (Czyni postać wyłączoną ze szczucia i celowania)
        setFaction(Faction::None); 
    }

    void MyCustomNPC::update(float dt) {
        Entity::update(dt);

        // Opcjonalne dodatki
    }
}
```

## System Podziału Kolizji dla Elementów Entity
Ponieważ zmodernizowaliśmy układy do spisu naturalnego rzutu bryłowego w grze 2.5D:
- Odrzucamy twarde fizyczne zderzaki (w starym systemie kwadraty wycelowane w środek ekranu).
- Od teraz fizyka postaci obmyśla jak wędrować w oparciu o koliste zderzenia między stopami z promieniem szerokości ciał wynoszącym umownie (~0.4 od punktu osi globalnej do granicy). Krawędzie wpadające na siebie obsuwają się z naturalną dynamiką po okręgu, nie spowalniając gry i czyniąc chodzenie jedwabiście gładkim.
- Wykorzystujemy precyzyjny silnik Mesh Hoveringu w wyliczaniu obrysów trafień, najeżdżania na klikalne postacie czy też podrzucanie informacji dla Broadfazy BoundingBoxowej używanej w skryptach rzutów atakujących pocisków.

### Dlaczego powinieneś wciąż wiedzieć o klasach dawnych Colliderów?
Elementy statyczne takie jak ściany ataku, niewidzialne trigger checkpoints przy wejściach do nowego rejonu, potężne pułapki rzutujące po długości lub kule armatnie w silniku bazują na sprawdzonych polach `CircleCollider` by wiedzieć, co obejmują. 
Nie instaluj ich pod ludzi, uciekaj z nimi pod `AbilityEffect`!

**Przykład:** Gracz nadstąpi na rejon ukrytego `CheckpointTriggera`. 
Checkpoint (Posiadający prostokąt RectangleCollider o długich krawędziach) poprosi o inspekcję Broadphase czy zderza się naturalnie z aktualnym 3D-obrysem pudła gracza (Bounding Box) wyodrębnionym metodą `_player->getBoundingBox()`.

### Pobieranie pozycji: `getCenter()` czy `getX()`?
- `getX() / getY()`: Zwraca fizyczny globalny punkt bytu w przestrzeni podłogi (W płaszczyźnie poziomej ziemi, zazwyczaj XZ w renderach 3D). Jest to grunt stopera, przydatny dla pathfindingu AI.
- `getCenter()`: **Główne Celowanie dla Walki**. Zwraca rzeczywisty ułamek osi położony na poziomie klatki piersiowej. Wykorzystuj ten promień jeśli wystrzeliwujesz Fireballa który ma lecieć wizualnie przed brzuch napastnika, a nie plątać mu się powierzchownie w cholewkach prawego buta!

### Debugowanie hitboksów testowych stref ataku
Aby spojrzeć na wizualne objęcie stożkowe swoich ataków wręcz z miecza na polu ekranowym (lub na pudełka broadfazy u Twoich potworów z silnika rzutów perspektywy) po prostu wpisz w kod:
```cpp
Nawia::Entity::Entity::DebugColliders = true;
```

## Renderowanie na Świecie z Iluzją Głębokości (Y-Sorting)

Kiedy wrzucisz Twoją postać do aktywnego wektora instancji do Managera silnika i nadejdzie faza rysowania, automatyczny obwód zdeklarowany w plikach rdzenia przejmie weryfikację pozycji postaci z osi pionowej. W procesie zwanym **Y-Sortingiem**, system układa figury stojące naturalnie dalej po linii jako pierwsze, malując modele po kolei aż pod samo czoło ekranu kamery i bohatera. Gwarantuje to stabilne prześwitywanie za wielogłowych potworów zza ich korpusu bez błędów 2D nachodzących obrysów!
