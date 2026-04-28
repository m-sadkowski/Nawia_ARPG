#include "ChestUI.h"
#include <GlobalScaling.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

namespace Nawia::UI
{

    ChestUI::ChestUI() {}

    void ChestUI::render(const Item::Backpack& chest_backpack, const Font& font) const 
    {
        const float inventory_start_x = Core::GlobalScaling::scaled(INV_START_X);
        const float inventory_start_y = Core::GlobalScaling::scaled(INV_START_Y);
        const float slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float inventory_width = Core::GlobalScaling::scaled(INV_WIDTH);
        const float inventory_height = Core::GlobalScaling::scaled(INV_HEIGHT);
        const float text_padding_left = Core::GlobalScaling::scaled(TEXT_PADDING_LEFT);
        const float text_padding_top = Core::GlobalScaling::scaled(TEXT_PADDING_TOP);
        const float font_size = Core::GlobalScaling::scaled(FONT_SIZE);

        // AAA Premium Background
        DrawRectangleRec({ inventory_start_x, inventory_start_y, inventory_width, inventory_height }, withAlpha(COLOR_PANEL_BG, 0.95f));
        DrawRectangleLinesEx({ inventory_start_x, inventory_start_y, inventory_width, inventory_height }, 2.0f, withAlpha(COLOR_ACCENT, 0.8f));
        DrawRectangleGradientV(static_cast<int>(inventory_start_x), static_cast<int>(inventory_start_y), static_cast<int>(inventory_width), static_cast<int>(inventory_height / 6.0f), withAlpha(WHITE, 0.05f), withAlpha(WHITE, 0.0f));

        DrawTextEx(font, "SKRZYNIA", { inventory_start_x + text_padding_left, inventory_start_y + text_padding_top }, font_size, 1.0f, COLOR_ACCENT);

        const Vector2 mouse_position = GetMousePosition();
        const float backpack_x = inventory_start_x + text_padding_left;
        const float backpack_y = inventory_start_y + text_padding_top + Core::GlobalScaling::scaled(50.0f);

        std::shared_ptr<Item::Item> item_tooltip = nullptr;
        Vector2 tooltip_position = { 0.0f, 0.0f };

        for (int i = 0; i < SLOT_AMOUNT; ++i)
        {
            const int column = i % COLS;
            const int row = i / COLS;

            const float slot_x = backpack_x + (column * (slot_size + Core::GlobalScaling::scaled(10.0f)));
            const float slot_y = backpack_y + (row * (slot_size + Core::GlobalScaling::scaled(10.0f)));

            const bool is_hovered = CheckCollisionPointRec(mouse_position, { slot_x, slot_y, slot_size, slot_size });
            const std::shared_ptr<Item::Item> item = (i < static_cast<int>(chest_backpack.getItems().size())) ? chest_backpack.getItems()[i] : nullptr;

            drawSlot(slot_x, slot_y, is_hovered, item);

            if (is_hovered && item != nullptr)
            {
                item_tooltip = item;
                tooltip_position = { mouse_position.x + 15.0f, mouse_position.y + 15.0f };
            }
        }

        if (item_tooltip != nullptr)
            drawTooltip(font, item_tooltip, tooltip_position.x, tooltip_position.y);
    }

    void ChestUI::drawTooltip(const Font& font, const std::shared_ptr<Item::Item>& item, float x, float y) const
    {
        const float font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const char* item_name = item->getName().c_str();

        const Vector2 text_size = MeasureTextEx(font, item_name, font_size, 1.0f);
        const float padding = 12.0f;

        DrawRectangleRec({ x, y, text_size.x + (padding * 2.0f), text_size.y + (padding * 2.0f) }, withAlpha(COLOR_PANEL_BG, 0.98f));
        DrawRectangleLinesEx({ x, y, text_size.x + (padding * 2.0f), text_size.y + (padding * 2.0f) }, 1.0f, COLOR_ACCENT);

        DrawTextEx(font, item_name, { x + padding, y + padding }, font_size, 1.0f, COLOR_GOLDEN_TEXT);
    }

    void ChestUI::drawSlot(const float x, const float y, const bool is_hovered, const std::shared_ptr<Item::Item>& item) const 
    {
        const float slot_padding = Core::GlobalScaling::scaled(SLOT_PADDING);
        const float slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);

        const Color background_color = is_hovered ? withAlpha(COLOR_ACCENT, 0.2f) : withAlpha(BLACK, 0.4f);
        const Color border_color = is_hovered ? COLOR_ACCENT : withAlpha(WHITE, 0.3f);
        
        DrawRectangleRec({ x, y, slot_size, slot_size }, background_color);
        DrawRectangleLinesEx({ x, y, slot_size, slot_size }, 1.0f, border_color);

        if (item != nullptr)
        {
            const Texture2D icon = item->getIcon();
            if (icon.id > 0)
            {
                const Rectangle source = { 0.0f, 0.0f, static_cast<float>(icon.width), static_cast<float>(icon.height) };
                const Rectangle destination = { x + slot_padding, y + slot_padding, slot_size - (slot_padding * 2.0f), slot_size - (slot_padding * 2.0f) };
                DrawTexturePro(icon, source, destination, { 0, 0 }, 0.0f, WHITE);
            }
        }
    }

    int ChestUI::handleInput() const 
    {
        const float slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float inventory_start_x = Core::GlobalScaling::scaled(INV_START_X);
        const float inventory_start_y = Core::GlobalScaling::scaled(INV_START_Y);
        const float text_padding_left = Core::GlobalScaling::scaled(TEXT_PADDING_LEFT);
        const float text_padding_top = Core::GlobalScaling::scaled(TEXT_PADDING_TOP);

        if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            return -1;

        const Vector2 mouse_position = GetMousePosition();
        const float backpack_x = inventory_start_x + text_padding_left;
        const float backpack_y = inventory_start_y + text_padding_top + Core::GlobalScaling::scaled(50.0f);

        for (int i = 0; i < SLOT_AMOUNT; ++i) 
        {
            const int column = i % COLS;
            const int row = i / COLS;
            const float slot_x = backpack_x + (column * (slot_size + Core::GlobalScaling::scaled(10.0f)));
            const float slot_y = backpack_y + (row * (slot_size + Core::GlobalScaling::scaled(10.0f)));

            if (CheckCollisionPointRec(mouse_position, { slot_x, slot_y, slot_size, slot_size }))
                return i;
        }

        return -1;
    }

} // namespace Nawia::UI