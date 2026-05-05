#pragma once

#include <Backpack.h>
#include <GlobalScaling.h>

#include <raylib.h>

#include <memory>

namespace Nawia::UI {

    class ChestUI {
    public:
        ChestUI();

        /** @brief Rysuje panel zawartosci skrzyni. */
        void render(const Item::Backpack& chest_backpack, const Font& font) const;

        /** @brief Zwraca klikniety slot skrzyni albo -1. */
        int handleInput() const;

    private:
        // Docelowo liczba slotow powinna wynikac z pojemnosci kontenera.
        static constexpr int COLS = 3;
        static constexpr int ROWS = 4;
        static constexpr int SLOT_AMOUNT = COLS * ROWS;
        static constexpr float SLOT_SIZE = 50.0f;
        static constexpr float SLOT_SPACING = 10.0f;
        static constexpr float SLOT_PADDING = 4.0f;
        static constexpr float INV_START_X = 600.0f;
        static constexpr float INV_START_Y = 50.0f;
        static constexpr float INV_WIDTH = 220.0f;
        static constexpr float INV_HEIGHT = 300.0f;
        static constexpr float TEXT_PADDING_LEFT = 20.0f;
        static constexpr float TEXT_PADDING_TOP = 10.0f;
        static constexpr float FONT_SIZE = 20.0f;

        /** @brief Oblicza prostokat slotu skrzyni na podstawie indeksu. */
        [[nodiscard]] Rectangle getSlotRect(int index) const;

        void drawSlot(Rectangle slot_rect, bool is_hovered, const std::shared_ptr<Item::Item>& item) const;
        void drawTooltip(const Font& font, const std::shared_ptr<Item::Item>& item, float x, float y) const;
    };
} // namespace Nawia::UI
