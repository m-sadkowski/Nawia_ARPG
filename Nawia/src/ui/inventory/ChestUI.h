#pragma once
#include <raylib.h>
#include <memory>
#include "Backpack.h"
#include <GlobalScaling.h>

namespace Nawia::UI {

    class ChestUI {
    public:
        ChestUI();

        void render(const Item::Backpack& chest_backpack, const Font& font) const;
        int handleInput() const;

    private:
        // todo dynamic slot amount
        static constexpr int SLOT_AMOUNT = 12;
        static constexpr int COLS = 3;
        static constexpr int ROWS = 4;
        static constexpr float SLOT_SIZE = 50.0f;
        static constexpr float PADDING = 10.0f;
        static constexpr float SLOT_PADDING = 4.0f;
        static constexpr float INV_START_X = 600.0f;
        static constexpr float INV_START_Y = 40.0f;
        static constexpr float INV_WIDTH = 220.0f;
        static constexpr float INV_HEIGHT = 300.0f;
        static constexpr float TEXT_PADDING_LEFT = 20.0f;
        static constexpr float TEXT_PADDING_TOP = 10.0f;
        static constexpr float FONT_SIZE = 20.0f;
        void drawSlot(float x, float y, bool is_hovered, const std::shared_ptr<Item::Item>& item) const;
        void drawTooltip(const Font& font, const std::shared_ptr<Item::Item>& item, float x, float y) const;
    };
}