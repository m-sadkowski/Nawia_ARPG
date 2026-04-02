# Nawia ARPG - Przewodnik Dewelopera

Ten dokument stanowi kompletny przewodnik po architekturze projektu Nawia. Opisuje hierarchię klas, nowy w pełni 3D system kolizji, tworzenie poziomów, przeciwników (Enemies) oraz umiejętności (Abilities).

## Spis Treści
1.  [Hierarchia Klas](#1-hierarchia-klas)
2.  [Tworzenie Map i Poziomów (Levels)](#2-tworzenie-map-i-poziomów-levels)
3.  [Tworzenie Nowego Entity](#3-tworzenie-nowego-entity)
4.  [Fizyka i System Kolizji 3D](#4-fizyka-i-system-kolizji-3d)
5.  [Tworzenie Przeciwników (Enemies)](#5-tworzenie-przeciwników-enemies)
6.  [System Umiejętności (Abilities)](#6-system-umiejętności-abilities)
7.  [Renderowanie i Y-Sorting](#7-renderowanie-i-y-sorting)

---

## 1. Hierarchia Klas

Podstawowym budulcem jest klasa `Nawia::Entity::Entity`.

*   **Entity**: Klasa bazowa. Posiada pozycję, teksturę/model, życie (HP), system animacji.
*   **EnemyInterface**: Dziedziczy po `Entity`. Rozszerza bazę o wskaźnik na `Map` (dla pathfindingu) i logikę specyficzną dla wrogów.
*   **Level**: Reprezentuje dany poziom, agregując obiekty na nim poprzez Managery.
*   **Map**: Odpowiada za wczytywanie pliku środowiska `.obj` oraz parametry nawigacji i sprawdzanie możliwości chodzenia (tzw. walkability, dawniej izometryczne płytki).
*   **Ability**: Klasa logiczna umiejętności (nie jest Entity). Odpowiada za "rzucenie" czaru (funkcja `cast`).
*   **AbilityEffect**: Dziedziczy po `Entity`. Reprezentuje fizyczny i geometryczny efekt ataku (np. promień pocisku, zakres cięcia mieczem).

---

## 2. Tworzenie Map i Poziomów (Levels)

Cały świat w Nawii wyrósł ze środowiska 2.5D i teraz operuje na **natywnym silniku 3D z perspektywą**. 
Aby stworzyć nowy poziom (Level) dla gracza:

1. Podłącz Menedżery: Powołaj klasę dziedziczącą ze standardowego poziomu (np. `DevLevel`).
2. Przygotuj plik mapy 3D (wyeksportuj model poziom/teren jako `.obj` z Blendera).
3. Podmień mapę w `DevLevel::init()` na nową i dostosuj jej pozycję głębokości (Offset Y):
    ```cpp
    _map.loadMap(engine, "../assets/models/twoja_mapa.obj");
    _map.setScale(0.04f);              // Ustaw domyślną skalę (jeśli model jest ogromny/mały)
    _map.setPosition(0.0f, -0.65f);    // Offset Y: obniż lub podnieś mapę 3D, by jej wizualny pułap "Ziemi" zgrał się z płaszczyzną Y=0 skryptów postaci. 
    ```
4. Wypełnij level w inicjalizatorze pożądanymi instancjami: np. zespawnuj NPC, dodaj skrzynki, postaw Devilów. Wszystkie współrzędne X oraz Y dla postaci są teraz globalnymi koordynatami w widoku top-down (gdzie silnikowe Y odzwierciedla w rzeczywistym świecie Oś Z).

---

## 3. Tworzenie Nowego Entity

Aby stworzyć nowy obiekt, stwórz klasę dziedziczącą po `Entity`.

### Konstrukcja i Modele 3D
W ciele konstruktora swojej klasy po prostu załaduj oryginalny i prawidłowy model 3D (format `.glb` ze wsparciem dla kości animacji) oraz wylistuj klipy.
Znacznie uproszczono proces - **nie trzeba już dodawać pod postać sztucznych colliderów**. Silnik jest w pełni 3D-świadomy.

```cpp
// Barrel.cpp
Barrel::Barrel(float x, float y, const std::shared_ptr<Texture2D>& tex)
    : Entity("Barrel", x, y, tex, 100)
{
    // Ładowanie precyzyjnego modelu 3D
    loadModel("../assets/models/barrel.glb");
    
    // Uruchomienie domyślnej zapętlonej animacji
    playAnimation("idle");
    
    // UWAGA:
    // playAnimation(name, loop, lock_movement, startFrame, force)
    // Przykład uderzenia z wymuszonym odtworzeniem:
    // playAnimation("get_hit", false, true, 0, true);
}
```

### Pozycjonowanie i Rotacja
*   `getX()`, `getY()`: Zwraca fizyczną pozycję 2D na logice mapy względem punktu styku butów / korzeni z glebą (Oś Y graficzna jest traktowana jako stała 0).
*   `getCenter()`: Zwraca środek modelu 3D (X oraz głębokość jako `Vector2`).
*   **Rotacja**: `rotateTowards(x, y)` obraca ciało bazując od stop po cel (do swobodnego chodzenia). Z kolei `rotateTowardsCenter(x, y)` upewnia się, by przy castingu promienie i rzuty ataków leciały z właściwego korpusu / torsu postaci.

---

## 4. Fizyka i System Kolizji 3D

Cały system opierał się dawniej na manualnych komponentach `RectangleCollider` umieszczanych w środku ciał, które przysparzały o ból głowy. System ewoluował na profesjonalny **pół-matematyczny ray-tracing po objętości 3D**:

### 1. Odpychanie Fizyczne (PushBox / Ruch)
Postacie, przeciwnicy i gracze automatycznie omijają się w tłumie (nie potrafią stać w jednym miejscu). Zapobiega to całkowicie błędom blokujących się rogów starożytnych, kwadratowych boxów.
Silnik sam dystansuje jednostki przy kolizjach fizycznych `EntityManager::resolveOverlap`, stosując wirtualny "dystans promienia" pomiędzy bazą obu ciał.

### 2. Walka, Umiejętności i Area of Effect (Precyzyjna Siatka/Mesh)
Wrogowane modele (nawet dla nie-hitscan pocisków) są analizowane rygorystycznie za sprawą dwuetapowej weryfikacji trafienia "pola w model", np. w ramach rzutu obszarowego (Cięcie mieczem):

1. **Broadphase**: W pierwszym kroku sprawdzane jest naturalne stykanie się pola z 3D `Bounding Boxem` wroga (Bądź ostrożny: `GetModelBoundingBox` na animowanych plikach `.glb` często bywa ogromne z winy wychylających kości klipów animacji atakujących).
2. **Raycast / Mesh Sweep**: Ze względu na duże gabaryty Bounding Boxa animacji, gdy cel pozytywnie przejdzie pretest broadphase, odbywa się **Ostateczny Mesh Check**. Silnik fizyczny miota ułamek wiązek laserowych w tors przeciwnika z identyczną matematyką raylib z jakiej korzystasz do precyzyjnego hoverowania (zaznaczania i wyczerniania postaci myszką!). 
    * Jeśli celnik minął pustą pustkę między nogami obrzydistnie pokracznego wroga – trafienie nie jest zaliczone!
    * System rozwiązuje ten test bezpośrednio, czy powierzchnia uderzyła w 3D siatkę, ignorując sztuczne i sztywne strefy przestrzenne.

### 3. Triggery 
Triggery obiektowe oparte o zdarzenia (np. wejście na ukrytego Checkpointa na podłodze) stosują inteligentną hybrydę sprawdzając własną skrzynkę nałożoną na Bounding Box gracza.

### Kwalifikatory Rejonu Efektów Umiejętności:
Służą **tylko** jako pole definiujące zasięg danego zaklęcia / ataku / triggera (Używać tylko w plikach efektów!):
*   **`RectangleCollider`**: Podłużne płaszczyzny i ściany ognia.
*   **`CircleCollider`**: Rzucany pocisk kulisty wędrujący by trafić w model (np. Fireball).
*   **`ConeCollider`**: Szerokie zamaszyste cięcia mieczem (SwordSlash).

---

## 5. Tworzenie Przeciwników (Enemies)

Klasa wrogów zapewnia własną autentyczną funkcję do pathfindingu omijającą statyczne przeszkody (wymaga `Map*` mapy).
Podpinaj dla wroga modele bazowe z sufixem typu `_idle.glb` a wszystkie poboczne operacje podpinaj pod konkretne String'i.

```cpp
Orc::Orc(float x, float y, std::shared_ptr<Texture2D> tex, Nawia::Core::Map* map)
    : EnemyInterface("Orc", x, y, tex, 200, map) // HP Wroga
{
    // Modele
    loadModel("../assets/models/orc.glb");
    addAnimation("run", "../assets/models/orc_run.glb");
    addAnimation("attack", "../assets/models/orc_attack.glb");
    
    // Od zaktualizowanej wersji 3D postać NIE narzuca sobie własnego collidera na plecy!
}
```

---

## 6. System Umiejętności (Abilities)

Rozdzielamy suche statystyki w czystym JSON od wysoce zoptymalizowanej logiki pętli C++.

### 1. Konfiguracja JSON
Edytuj `assets/data/abilities.json`:
```json
{
  "name": "SuperSlash",
  "stats": {
    "damage": 50,
    "cooldown": 1.5,
    "cast_range": 2.0,
    "hitbox_radius": 2.5,
    "duration": 0.2
  }
}
```

### 2. Wykonywane Skrypty i Efekty
Obiekt typu `AbilityEffect` jest rzucany w przestrzeń `EntityManager` od wywołania go przez klasę matkę Ability. Implementuje unikalny efekt wizualny w swoim obrysie kolizyjnym. 
Aby nadać mu ten obrys, wykrój mu promień rany np. stożek, a jego bazowa metoda na każdej nowej klatce zatroszczy się (patrz Punkt 4) o wyłuskanie wszystkich żywych zaszczepionych celów modelowych ze strefy Mesh Sweepingu i aplikowanie na celach funkcji trafień:

```cpp
// wewnątrz skryptu Effectu np. SwordSlashEffect::onCollision(const std::shared_ptr<Entity>& target)
void SwordSlashEffect::onCollision(const std::shared_ptr<Entity>& target)
{
    // Gwarancja zaatakowania odpowiedniej rasy, nie zranienia przyjacielskich npc.
    if (auto enemy = std::dynamic_pointer_cast<EnemyInterface>(target))
    {
        // Pamięć przebytych trafionych zapobiega multi-hitom od jednej fazy efektu rzutu
        if (!hasHit(enemy)) {
            enemy->takeDamage(getDamage());
            addHit(enemy); 
        }
    }
}
```

---

## 7. Renderowanie i Y-Sorting

Całe malowanie scenerii oparte o render-liste wewnątrz `EntityManager::renderEntities` posiada wbudowane zarządzanie głębią metodą **Y-Sorting**:
- Zależnie od tego z jakiej perspektywicznej odległości patrzy kamera przestrzenna 3D na płasko renderowaną przestrzeń.
- Wykorzystuje wewnętrznie element sortujący stawiając byty wizualne dalej na Osi głębokości jako bazę tła obrysowego od postaci które stoją w linii niższej z racji kamery.
To gwarantuje gładkie perspektywalne przykrywanie się i brak nachodzenia pikseli 2D sprite'ów czy dziwnych zachowań głębi modeli.
