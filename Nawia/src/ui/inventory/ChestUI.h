#pragma once

#include <Backpack.h>
#include <GlobalScaling.h>

#include <raylib.h>

#include <memory>

namespace Nawia::Core {
    class ResourceManager;
}

namespace Nawia::UI {

    /**
     * @class ChestUI
     * @brief Rysuje zawartosc otwartej skrzyni i obsluguje klikniecia slotow.
     */
    class ChestUI {
    public:
        ChestUI();

        /** @brief Laduje tlo panelu skrzyni. */
        void loadResources(Core::ResourceManager& resource_manager);

        /** @brief Rysuje panel zawartosci skrzyni. */
        void render(const Item::Backpack& chest_backpack, const Font& font) const;

        /** @brief Zwraca klikniety slot skrzyni albo -1. */
        int handleInput() const;

    private:
        // Docelowo liczba slotow powinna wynikac z pojemnosci kontenera.
        static constexpr int COLS = 3;
        static constexpr int ROWS = 4;
        static constexpr int SLOT_AMOUNT = COLS * ROWS;
        static constexpr float SLOT_PADDING = 4.0f;
        static constexpr float INV_START_X = 800.0f;
        static constexpr float INV_START_Y = 96.0f;
        static constexpr float INV_WIDTH = 300.0f;
        static constexpr float INV_HEIGHT = 452.0f;
        static constexpr float FONT_SIZE = 20.0f;

        std::shared_ptr<Texture2D> _background;

        /** @brief Oblicza prostokat panelu skrzyni. */
        [[nodiscard]] Rectangle getPanelRect() const;

        /** @brief Oblicza prostokat slotu skrzyni na podstawie indeksu. */
        [[nodiscard]] Rectangle getSlotRect(int index) const;

    };
} // namespace Nawia::UI
