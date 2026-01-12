# Przewodnik po obiektach Interaktywnych

Obiekty interaktywne to elementy świata, z którymi gracz może wchodzić w interakcję (klikać na nie lub wchodzić w nie).
Najczęstszym przypadkiem są obiekty "Klikalne", takie jak Skrzynie, NPC, czy Dźwignie.

## Klasa Bazowa: `InteractiveClickable`

Klasa `Nawia::Entity::InteractiveClickable` dziedziczy po `Entity` (ma pozycję, grafikę) oraz `Interactable` (interfejs interakcji).

### Interfejs `Interactable`
```cpp
virtual void onInteract(Entity& instigator) = 0;   // Co się dzieje po kliknięciu/interakcji
virtual void onTriggerEnter(Entity& other) = 0;    // Co się dzieje po wejściu w collider
virtual bool canInteract() const;                  // Czy można teraz użyć (np. zablokowane drzwi)
```

## Jak stworzyć klikalną skrzynię?

Należy odziedziczyć po `InteractiveClickable`.

```cpp
#include "InteractiveClickable.h"
#include "Collider.h" // Poprawny include

namespace Nawia::Entity {

    class TreasureChest : public InteractiveClickable {
    public:
        TreasureChest(float x, float y) 
            : InteractiveClickable("Chest", x, y, nullptr, 1) // 1 HP (niezniszczalne logicznie)
        {
            // Ładowanie modelu 3D
            loadModel("assets/models/chest.glb");
            addAnimation("closed", "assets/animations/chest_closed.glb");
            addAnimation("open", "assets/animations/chest_open.glb");
            
            // Collider jest KONIECZNY, aby wykryć kliknięcie myszką!
            // Prostokąt 40x40, bez offsetu (0,0) - chyba że model jest przesunięty.
            setCollider(std::make_unique<RectangleCollider>(this, 40, 40, 0.0f, 0.0f));
            
            setFaction(Faction::Neutral); // Obiekty interaktywne zazwyczaj są neutralne.
            
            playAnimation("closed");
        }

        // Ta metoda jest wywoływana przez PlayerController, gdy gracz kliknie na obiekt i podejdzie blisko
        void onInteract(Entity& instigator) override {
            if (_is_open) return; // Już otwarta

            _is_open = true;
            playAnimation("open", false); // Odtwórz raz
            
            std::cout << "Skrzynia otwarta przez " << instigator.getName() << "!" << std::endl;
        }

        // Przydatne do debugowania: włącz Entity::DebugColliders = true aby widzieć obszar klikalny.

    private:
        bool _is_open = false;
    };
}
```

## Ważne uwagi

1. **Collider**: Bez collidera metoda `isMouseOver` (używana do detekcji kliknięcia) nie zadziała. Musisz ustawić collider (`Box` lub `Circle`). Jeśli klikanie "nie wchodzi", sprawdź offsety collidera w trybie debugowania.
2. **Interactable vs Trigger**:
   - Jeśli chcesz obiekt, na który się **Klika** (NPC, Skrzynia): dziedzicz po `InteractiveClickable` i implementuj `onInteract`.
   - Jeśli chcesz obiekt, który działa **po wejściu** (Pułapka, Teleport): dziedzicz po `Entity` i `Interactable` (lub stwórz nadrzędną klasę `TriggerBase`) i implementuj `onTriggerEnter`.
     - *Uwaga*: Domyślny `InteractiveClickable` ma pusty `onTriggerEnter`.

3. **Zasięg interakcji**: To kontroler gracza (`PlayerController`) zazwyczaj decyduje, z jakiej odległości można wejść w interakcję. Obiekt interaktywny tylko odpowiada na wywołanie `onInteract`.
