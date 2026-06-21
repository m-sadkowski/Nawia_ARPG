# Dokumentacja techniczna projektu - DTP
**Nazwa i akronim projektu:** Gra komputerowa z półotwartym światem (Nawia_ARPG)
**Nr wersji:** 1.01
**Data ostatniej aktualizacji:** 15.06.2026

## Historia zmian
| Wersja | Opis modyfikacji | Rozdział / strona | Autor modyfikacji | Data |
|--------|------------------|-------------------|-------------------|------|
| 1.00   | Wersja wstępna   | całość            | Matysiak, Michał  | 24.01.2026 |
| 1.01   | Aktualizacja stanu projektu (nowe moduły, zewnętrzne biblioteki, audio, systemy UI i AI) | 2.1, 2.2, 2.3 | Antigravity AI | 15.06.2026 |

---

## 1 Wprowadzenie - o dokumencie

### 1.1 Cel dokumentu
Celem dokumentu jest udokumentowanie informacji dotyczących produktu, jego cech funkcjonalnych, parametrów technicznych, schematów blokowych, oprogramowania, wyników działania, zdjęć produktu, pomiarów, testów oraz innych elementów wymaganych przez opiekuna i klienta.

### 1.2 Zakres dokumentu
W zakres dokumentu wchodzi opis architektury systemu, wykorzystanych bibliotek, struktury klas oraz mechanik rozgrywki zaimplementowanych w projekcie. Dokument ten został zaktualizowany na bieżąco względem wersji styczniowej i oddaje aktualny stan gry.

### 1.3 Odbiorcy
Odbiorcą tego dokumentu jest opiekun projektu dr inż. Przemysław Falkowski-Gilski oraz koordynator studenckich projektów grupowych w Katedrze Systemów Geoinformatycznych dr inż. Krzysztof Bikonis.

### 1.4 Terminologia
* **Entity (Encja)** - Podstawowy obiekt w świecie gry, mogący reprezentować gracza, przeciwnika, sojusznika lub element otoczenia.
* **Raylib** - Biblioteka programistyczna wykorzystywana do obsługi grafiki, okna aplikacji oraz wejścia użytkownika.
* **ARPG (Action Role-Playing Game)** - Gatunek gry łączący elementy zręcznościowe z rozwojem postaci.
* **Collider** - Komponent odpowiedzialny za wykrywanie kolizji między obiektami.
* **Checkpoint** - (punkt kontrolny) w grach to z góry ustalone miejsce, w którym następuje automatyczny lub ręczny zapis postępów.
* **Singleton** - kreacyjny wzorzec projektowy, który gwarantuje istnienie tylko jednej instancji danej klasy w aplikacji.
* **Renderowanie** - proces przekształcania danych cyfrowych w końcowy, wizualny lub dźwiękowy rezultat.
* **NavMesh (Navigation Mesh)** - Struktura danych wykorzystywana do wyszukiwania ścieżek przez sztuczną inteligencję (przeciwników i NPC).

## 2 Dokumentacja techniczna projektu

### 2.1 Środowisko uruchomieniowe:
**Język programowania i technologie:**
Projekt został zaimplementowany w języku C++ (standard C++20) i jest budowany przy użyciu CMake (wersja 3.20+). W projekcie używane są następujące biblioteki (pobierane i integrowane przy pomocy CMake FetchContent):
* **Raylib 5.5**: Główna biblioteka do renderowania grafiki 2D/3D oraz obsługi urządzeń wejścia (klawiatura, mysz).
* **TinyXML2**: Szybki parser XML wykorzystywany specjalnie do wczytywania kafelkowych map tworzonych w programie Tiled (z plików o rozszerzeniu `.tsx`).
* **Recast Navigation (Recast & Detour)**: Potężna zewnętrzna biblioteka odpowiedzialna za sztuczną inteligencję i odnajdywanie drogi (NavMesh).
* **nlohmann/json**: Do parsowania i zapisywania plików konfiguracyjnych i zapisów gry (`external/json`).

### 2.2 Komponenty aplikacji

Obecna architektura aplikacji jest mocno rozbudowana w stosunku do pierwotnych założeń i została podzielona na następujące moduły:

#### 2.2.1 Szkielet silnika (`src/core`)
Klasa `Engine` stanowi serce aplikacji, zarządzając główną pętlą gry, inicjalizacją podsystemów (okno, audio, renderer) oraz zwalnianiem zasobów przy zamknięciu. W ramach silnika wyodrębniono także m.in. `GlobalScaling` do zarządzania proporcjami grafiki na różnych rozdzielczościach.

#### 2.2.2 Zarządzanie rozgrywką (`src/core/game`)
* **EntityManager**: Centralny menedżer wszystkich obiektów w grze. Odpowiada za ich aktualizację, renderowanie oraz cykl życia.
* **Map / Camera**: Systemy obsługi wczytywania i renderowania tła oraz podążania kamery za graczem.
* **DialogueManager & StoryConditions**: Menedżery sterujące przebiegiem dialogów oraz postępami fabuły/zadań (questów).
* **Questy i Bossowie**: Specjalistyczna logika wywoływania walk z bossami oraz zarządzania zadaniami dla gracza (katalogi `boss`, `quest`, `save`).

#### 2.2.3 Podsystemy (`src/core/system`)
Kod odpowiedzialny za warstwę sprzętową i konfigurację:
* **Settings**: Zarządzanie globalnymi ustawieniami gry (rozdzielczość, głośność, przypisanie klawiszy).
* **Input**: Abstrakcja warstwy wejścia, obsługująca klawiaturę i mysz.
* **Renderer**: Abstrakcja warstwy graficznej ułatwiająca rysowanie i zarządzenie oknem.
* **Time**: Nowy moduł odpowiedzialny za pomiary czasu gry (liczenie delty, opóźnienia, timery).

#### 2.2.4 Narzędzia (`src/core/util`)
* **ResourceManager / AssetPreloader / AssetLoadManifest**: Zaawansowany system do wstępnego wczytywania oraz zarządzania pamięcią zasobów (tekstury, dźwięki).
* **Logger**: System logowania zdarzeń (np. do pliku lub konsoli) wspierający debugowanie.
* **MathUtils**: Zbiór funkcji matematycznych wykorzystywanych przez inne systemy.

#### 2.2.5 System encji (`src/entity`)
* Architektura zmierza do większej modularyzacji wprowadzając oddzielne moduły na logikę zachowań i danych: **Components** (dane) oraz **Systems** i **Effects** (systemy i efekty wizualne).
* **Entity**: Główna klasa bazowa, od której dziedziczą obiekty na mapie.
* **Actors**: Postacie w świecie gry, rozdzielone na:
  * Gracza (`player`), 
  * Różnorodnych przeciwników (`enemies`): m.in. `walking_dead`, `devil`, `bandit`,
  * Postacie poboczne i sojuszników (`allies`): m.in. postać `friend`.
* **Abilities**: Rozbudowany system zdolności, wspierający konkretne ataki wręcz (np. `SwordSlash`) oraz pociski (np. `Fireball`, `KnifeThrow`). Ataki są m.in. generowane za pomocą `PlayerAbilityFactory`.
* **Collider**: Wyspecjalizowana detekcja kolizji między poszczególnymi obiektami.
* **Interactive**: Obiekty reagujące na akcje podzielono na te wykrywające wejście w obszar kolizji (`onTrigger`) oraz te reagujące na wejście użytkownika (`clickable` m.in. `chest` (skrzynie), `NPC` do dialogów).

#### 2.2.6 Świat Gry (`src/world`)
* **Level / Map**: Ładowanie poziomów oraz interpretacja metadanych kafelków i tła, wspierane przez TinyXML2.
* **NavMesh**: Moduł oparty na bibliotece Recast/Detour, umożliwiający dynamiczne odnajdywanie drogi przez postacie niezależne (ścieżkowanie i omijanie przeszkód).
* **Spawn**: System dynamicznego generowania obiektów, wrogów i punktów spawnu na poszczególnych poziomach.

#### 2.2.7 Interfejs Użytkownika (`src/ui`)
* **UIHandler**: Centralny komponent spinający wyświetlanie różnych okien i interfejsów gry.
* **HUD & StatsUI**: Wyświetlanie bieżących statystyk życia, punktów many oraz poziomu podczas gry.
* **Inventory / Quest / Dialogue**: Rozbudowane widoki dla ekwipunku gracza, dziennika zadań oraz dedykowane okna interakcji i rozmowy z NPC.
* **Menu**: Ekran menu głównego gry.
* **CustomCursor**: Niestandardowy kursor wyświetlany w grze.

#### 2.2.8 Przedmioty (`src/item`)
* **ItemDatabase**: System bazy danych konfigurujący i przetrzymujący informacje o wszystkich dostępnych typach przedmiotów.
* **Item**: System instancji przedmiotów oraz logiki ich wpływu na gracza (katalogi `inventory`, `types`, `utils`).

#### 2.2.9 Dźwięk (`src/audio`)
* **AudioManager**: Odpowiada za odtwarzanie efektów dźwiękowych (SFX) oraz muzyki w tle, a także za globalne zarządzanie głośnością. Używa zestawu indentyfikatorów dźwięków.

### 2.3 Schemat funkcjonalny aplikacji:

#### 2.3.1 Pętla Główna
Inicjalizacja systemu (m.in. Okno, Audio, Settings) → Wczytanie zasobów (`AssetPreloader`) → Pętla gry (`Update` wspierane przez system w modułach `Time` oraz `EntitySystems` → `Draw` interfejsu i świata) → Zamknięcie aplikacji i bezpieczne zwolnienie pamięci.

#### 2.3.2 Ekran Główny
Opcje:
1. **Rozpocznij Grę / Kontynuuj**: Wejście do rozgrywki oraz wczytanie stanu z systemu zapisu/odczytu (`nlohmann/json`).
2. **Ustawienia**: Zmiana m.in głośności i rozdzielczości (skalowanie dzięki `GlobalScaling`).
3. **Wyjście**: Wyłączenie programu.

#### 2.3.3 Rozgrywka
* **Sterowanie postacią**: Poruszanie się postacią (np. klawiszami WSAD lub myszą), ataki podstawowe, interakcje z obiektami (`clickable` i `onTrigger`) obsługiwane przez lewy przycisk myszy / klawiaturę.
* **Umiejętności specjalne**: Korzystanie z umiejętności typu `Fireball` czy rzut nożem po przypisaniu do klawiszy akcji (np. Q, W, E, R).
* **AI i system Walki**: Wrogowie wykorzystują `NavMesh` aby podążać za graczem, detekcja kolizji wykorzystuje `Collider`. Pojawił się dedykowany moduł obsługi skryptów Bossów.
* **Fabuła**: Nowy silnik postępu wydarzeń kontrolowany przez `DialogueManager` oraz zbiór flag `StoryConditions`.

#### 2.3.4 Zarządzanie Postacią
* **Ekwipunek**: Zbieranie i przemieszczanie przedmiotów wewnątrz interfejsu (otwierane z reguły klawiszem "I"), co ma bezpośredni wpływ na statystyki gracza (np. szybkość, punkty zdrowia).
* **Questy (Nowość)**: Możliwość przeglądania podjętych oraz ukończonych zadań.

## 3 Załączniki
| L.p. | Nazwa dokumentu | Nazwa pliku |
|---|---|---|
| 1. | Kod źródłowy Nawia | Nawia_ARPG-v1.01.zip |
