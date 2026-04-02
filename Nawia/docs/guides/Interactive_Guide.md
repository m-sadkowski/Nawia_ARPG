# Przewodnik po obiektach Interaktywnych

Obiekty interaktywne to elementy świata, z którymi gracz może wchodzić w interakcję (klikać na nie lub wchodzić w nie).
Najczęstszym przypadkiem są obiekty "Klikalne", takie jak Skrzynie, NPC, czy Dźwignie, a także triggery takie jak Strefy Ognia.

## Klasa Bazowa: `InteractiveClickable`

Klasa `Nawia::Entity::InteractiveClickable` dziedziczy po `Entity` (ma pozycję, wczytuje podpiętą grafikę/model 3d) oraz `Interactable` (interfejs interakcji).

### Interfejs `Interactable`
```cpp
virtual void onInteract(Entity& instigator) = 0;   // Co się dzieje po kliknięciu/interakcji (Tylko dla obiektów typu InteractiveClickable)
virtual void onTriggerEnter(Entity& other) = 0;    // Co się dzieje po natychmiastowym wdepnięciu na obiekt fizycznie
virtual bool canInteract() const;                  // Czy można teraz użyć (np. zablokowane drzwi)
```

## Jak stworzyć klikalną skrzynię?

Należy odziedziczyć po `InteractiveClickable`.

```cpp
#include "InteractiveClickable.h"

namespace Nawia::Entity {

    class TreasureChest : public InteractiveClickable {
    public:
        TreasureChest(float x, float y) 
            : InteractiveClickable("Chest", x, y, nullptr, 1) // 1 HP (niezniszczalne logicznie)
        {
            // Ładowanie precyzyjnego modelu 3D
            loadModel("assets/models/chest.glb");
            addAnimation("closed", "assets/animations/chest_closed.glb");
            addAnimation("open", "assets/animations/chest_open.glb");
            
            setFaction(Faction::Neutral); // Obiekty interaktywne zazwyczaj są neutralne by nie były raniowane czarami walki
            
            playAnimation("closed");
        }

        // Ta metoda jest wywoływana przez PlayerController, gdy gracz kliknie NA SIATKĘ OBIEKTU 3D i podejdzie blisko
        void onInteract(Entity& instigator) override {
            if (_is_open) return; // Już otwarta

            _is_open = true;
            playAnimation("open", false); // Odtwórz raz
            
            std::cout << "Skrzynia otwarta przez " << instigator.getName() << "!" << std::endl;
        }

    private:
        bool _is_open = false;
    };
}
```

## Ważne uwagi z silnika Pół-3D

1. **System Wykrywania Przez Myszke:** Wskazówka 3D nie potrzebuje dodawania żadnych ręcznych wirtualnych brył pod teksturę! Przez najnowszą aktualizację silnika do operacji o 3D Meshe, myszka wystrzeliwuje `Raycast` natywnie. Jeżeli najedziesz kursorem obok tekstury na puste miejsce – skrzynia się nie podświetli. Najedź starannie dokładnie na róg wieka, to postać się przyciemnia!
2. **Interactable vs Trigger**:
   - Jeśli chcesz obiekt, na który gracz z własnej woli **Klika** kursorem (Otwiera skrzynie, zagaduj NPC): dziedziczysz bezwzględnie w oparciu o obiekt klasy `InteractiveClickable` i implementujesz funkcję `onInteract`.
   - Jeśli chcesz niewidzialny Trigger strefowy na podłodze, np. zapadający po wdepnięciu przez bohatera: musisz używać dziedziczenia pod `InteractiveTrigger`. Ten wciąż potrzebuje definicji Collidera (na przykład rzucając mu płaski RectangleCollider na podłogę)! Strefa z Colliderem pod Trigger natywnie wykryje czy gruba animacja Bounding Boxu Gracza stanęła na niego w systemie Broadphase i wywoła na Tobie publiczną metodę `onTriggerEnter`.

3. **Zasięg interakcji**: Pamiętaj by dla własnych skrzyń nadpisać float `getInteractionRange()` (Obecnie w skrzyni to pole 6.25 z dystansu promienia logowania). To PlayerController decyduje, jak długo Twoja postać idzie do klikniętej wcześniej przedmiotu a następnie wykonuje akcję zapisaną w onInteract gdy jej promień dotknie wymaganego ułamka.
