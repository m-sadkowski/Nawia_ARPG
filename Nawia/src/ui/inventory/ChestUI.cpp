#include "ChestUI.h"

namespace Nawia::UI {

	ChestUI::ChestUI() {}

	void ChestUI::render(const Item::Backpack& chest_backpack, const Font& font) const 
	{
		const float inv_start_x = Core::GlobalScaling::scaled(INV_START_X);
		const float inv_start_y = Core::GlobalScaling::scaled(INV_START_Y);
        const float slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
		const float inv_width = Core::GlobalScaling::scaled(INV_WIDTH);
		const float inv_height = Core::GlobalScaling::scaled(INV_HEIGHT);
		const float text_padding_left = Core::GlobalScaling::scaled(TEXT_PADDING_LEFT);
		const float text_padding_top = Core::GlobalScaling::scaled(TEXT_PADDING_TOP);
		const float font_size = Core::GlobalScaling::scaled(FONT_SIZE);

		DrawRectangle(inv_start_x, inv_start_y, inv_width, inv_height, Fade(BLACK, 0.9f));
		DrawRectangleLines(inv_start_x, inv_start_y, inv_width, inv_height, WHITE);

		DrawTextEx(font, "CHEST", { inv_start_x + text_padding_left, inv_start_y + text_padding_top }, font_size, 1.0f, WHITE);

        const Vector2 mouse_pos = GetMousePosition();

        const float backpack_x = inv_start_x + text_padding_left;
        const float backpack_y = inv_start_y + text_padding_top + 50;

        std::shared_ptr<Item::Item> item_tooltip = nullptr;
        Vector2 tooltip_pos = { 0, 0 };

        for (int i = 0; i < SLOT_AMOUNT; ++i) {
            const int col = i % COLS;
            const int row = i / COLS;

            const float slot_x = backpack_x + (col * (slot_size + Core::GlobalScaling::scaled(10.0f)));
            const float slot_y = backpack_y + (row * (slot_size + Core::GlobalScaling::scaled(10.0f)));

            const bool is_hovered = CheckCollisionPointRec(mouse_pos, { slot_x, slot_y, slot_size, slot_size });

            std::shared_ptr<Item::Item> item = (i < chest_backpack.getItems().size()) ? chest_backpack.getItems()[i] : nullptr;

            drawSlot(slot_x, slot_y, is_hovered, item);

            if (is_hovered && item != nullptr) {
                item_tooltip = item;
                tooltip_pos = { mouse_pos.x + 15, mouse_pos.y + 15 };
            }
        }

        if (item_tooltip != nullptr) {
            drawTooltip(font, item_tooltip, tooltip_pos.x, tooltip_pos.y);
        }
	}

    void ChestUI::drawTooltip(const Font& font, const std::shared_ptr<Item::Item>& item, float x, float y) const {
        const float font_size = Core::GlobalScaling::scaled(FONT_SIZE);

        const char* item_name = item->getName().c_str();

        const Vector2 text_size = MeasureTextEx(font, item_name, font_size, 1.0f);
        constexpr float padding = 8.0f;

        DrawRectangle(x, y, text_size.x + (padding * 2), text_size.y + (padding * 2), Fade(BLACK, 0.9f));
        DrawRectangleLines(x, y, text_size.x + (padding * 2), text_size.y + (padding * 2), WHITE);

        DrawTextEx(font, item_name, { x + padding, y + padding }, font_size, 1.0f, YELLOW);
    }

    void ChestUI::drawSlot(const float x, const float y, const bool is_hovered, const std::shared_ptr<Item::Item>& item) const 
	{
        const float slot_padding = Core::GlobalScaling::scaled(SLOT_PADDING);
        const float slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const float padding = Core::GlobalScaling::scaled(PADDING);

        // slot background
        const Color slot_color = is_hovered ? LIGHTGRAY : DARKGRAY;
        DrawRectangle(x, y, slot_size, slot_size, slot_color);
        DrawRectangleLines(x, y, slot_size, slot_size, WHITE);

        if (item != nullptr) {
            const Texture2D icon = item->getIcon();

            // check if texture is loaded
            if (icon.id > 0) {
                const Rectangle source = { 0.0f, 0.0f, static_cast<float>(icon.width), static_cast<float>(icon.height) };

                const Rectangle dest = {
                    x + slot_padding,
                    y + slot_padding,
                    slot_size - (slot_padding * 2),
                    slot_size - (slot_padding * 2)
                };

                DrawTexturePro(icon, source, dest, { 0, 0 }, 0.0f, WHITE);
            }
        }
    }

    int ChestUI::handleInput() const 
	{
        const float slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float inv_start_x = Core::GlobalScaling::scaled(INV_START_X);
        const float inv_start_y = Core::GlobalScaling::scaled(INV_START_Y);
        const float text_padding_left = Core::GlobalScaling::scaled(TEXT_PADDING_LEFT);
        const float text_padding_top = Core::GlobalScaling::scaled(TEXT_PADDING_TOP);

        if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            return -1;

        const Vector2 mouse_pos = GetMousePosition();

        const float backpack_x = inv_start_x + text_padding_left;
        const float backpack_y = inv_start_y + text_padding_top + 50;

        for (int i = 0; i < SLOT_AMOUNT; ++i) 
        {
            const int col = i % COLS;
            const int row = i / COLS;
            const float slot_x = backpack_x + (col * (slot_size + Core::GlobalScaling::scaled(10.0f)));
            const float slot_y = backpack_y + (row * (slot_size + Core::GlobalScaling::scaled(10.0f)));

            const Rectangle slot_rect = { slot_x, slot_y, slot_size, slot_size };

            if (CheckCollisionPointRec(mouse_pos, slot_rect)) {
                return i;
            }
        }

        return -1;
    }

}