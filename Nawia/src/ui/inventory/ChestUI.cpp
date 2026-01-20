#include "ChestUI.h"

namespace Nawia::UI {

	ChestUI::ChestUI() {}

	void ChestUI::render(const Item::Backpack& chestBackpack, const Font& font) const {
		const float _inv_start_x = Core::GlobalScaling::scaled(INV_START_X);
		const float _inv_start_y = Core::GlobalScaling::scaled(INV_START_Y);
        const float _slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
		const float _inv_width = Core::GlobalScaling::scaled(INV_WIDTH);
		const float _inv_height = Core::GlobalScaling::scaled(INV_HEIGHT);
		const float _text_padding_left = Core::GlobalScaling::scaled(TEXT_PADDING_LEFT);
		const float _text_padding_top = Core::GlobalScaling::scaled(TEXT_PADDING_TOP);
		const float _font_size = Core::GlobalScaling::scaled(FONT_SIZE);

		DrawRectangle(_inv_start_x, _inv_start_y, _inv_width, _inv_height, Fade(BLACK, 0.9f));
		DrawRectangleLines(_inv_start_x, _inv_start_y, _inv_width, _inv_height, WHITE);

		DrawTextEx(font, "CHEST", { _inv_start_x + _text_padding_left, _inv_start_y + _text_padding_top }, _font_size, 1.0f, WHITE);

        const Vector2 mouse_pos = GetMousePosition();

        const float backpack_x = _inv_start_x + _text_padding_left;
        const float backpack_y = _inv_start_y + _text_padding_top + 50;

        std::shared_ptr<Item::Item> item_tooltip = nullptr;
        Vector2 tooltip_pos = { 0, 0 };

        for (int i = 0; i < SLOT_AMOUNT; ++i) {
            const int col = i % COLS;
            const int row = i / COLS;

            const float slot_x = backpack_x + (col * (_slot_size + Core::GlobalScaling::scaled(10.0f)));
            const float slot_y = backpack_y + (row * (_slot_size + Core::GlobalScaling::scaled(10.0f)));

            const bool is_hovered = CheckCollisionPointRec(mouse_pos, { slot_x, slot_y, _slot_size, _slot_size });

            std::shared_ptr<Item::Item> item = (i < chestBackpack.getItems().size()) ? chestBackpack.getItems()[i] : nullptr;

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
        const float _font_size = Core::GlobalScaling::scaled(FONT_SIZE);

        const char* _item_name = item->getName().c_str();

        const Vector2 text_size = MeasureTextEx(font, _item_name, _font_size, 1.0f);
        const float padding = 8.0f;

        DrawRectangle(x, y, text_size.x + (padding * 2), text_size.y + (padding * 2), Fade(BLACK, 0.9f));
        DrawRectangleLines(x, y, text_size.x + (padding * 2), text_size.y + (padding * 2), WHITE);

        DrawTextEx(font, _item_name, { x + padding, y + padding }, _font_size, 1.0f, YELLOW);
    }

    void ChestUI::drawSlot(float x, float y, bool isHovered, const std::shared_ptr<Item::Item>& item) const {
        const float _slot_padding = Core::GlobalScaling::scaled(SLOT_PADDING);
        const float _slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float _font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const float _padding = Core::GlobalScaling::scaled(PADDING);

        // slot background
        const Color slot_color = isHovered ? LIGHTGRAY : DARKGRAY;
        DrawRectangle(x, y, _slot_size, _slot_size, slot_color);
        DrawRectangleLines(x, y, _slot_size, _slot_size, WHITE);

        if (item != nullptr) {
            const Texture2D icon = item->getIcon();

            // check if texture is loaded
            if (icon.id > 0) {
                const Rectangle source = { 0.0f, 0.0f, (float)icon.width, (float)icon.height };

                const Rectangle dest = {
                    x + _slot_padding,
                    y + _slot_padding,
                    _slot_size - (_slot_padding * 2),
                    _slot_size - (_slot_padding * 2)
                };

                DrawTexturePro(icon, source, dest, { 0, 0 }, 0.0f, WHITE);
            }
        }
    }

    int ChestUI::handleInput() const {
        const float _slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float _inv_start_x = Core::GlobalScaling::scaled(INV_START_X);
        const float _inv_start_y = Core::GlobalScaling::scaled(INV_START_Y);
        const float _text_padding_left = Core::GlobalScaling::scaled(TEXT_PADDING_LEFT);
        const float _text_padding_top = Core::GlobalScaling::scaled(TEXT_PADDING_TOP);

        if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            return -1;
        }

        const Vector2 mouse_pos = GetMousePosition();

        const float backpack_x = _inv_start_x + _text_padding_left;
        const float backpack_y = _inv_start_y + _text_padding_top + 50;

        for (int i = 0; i < SLOT_AMOUNT; ++i) {
            const int col = i % COLS;
            const int row = i / COLS;
            const float slotX = backpack_x + (col * (_slot_size + Core::GlobalScaling::scaled(10.0f)));
            const float slotY = backpack_y + (row * (_slot_size + Core::GlobalScaling::scaled(10.0f)));

            const Rectangle slot_rect = { slotX, slotY, _slot_size, _slot_size };

            if (CheckCollisionPointRec(mouse_pos, slot_rect)) {
                return i;
            }
        }

        return -1;
    }

}