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

        Vector2 mousePos = GetMousePosition();

        float backpackX = _inv_start_x + _text_padding_left;
        float backpackY = _inv_start_y + _text_padding_top + 50;

        std::shared_ptr<Item::Item> itemTooltip = nullptr;
        Vector2 tooltipPos = { 0, 0 };

        for (int i = 0; i < 12; ++i) {
            int col = i % COLS;
            int row = i / COLS;

            float slotX = backpackX + (col * (_slot_size + Core::GlobalScaling::scaled(10.0f)));
            float slotY = backpackY + (row * (_slot_size + Core::GlobalScaling::scaled(10.0f)));

            bool isHovered = CheckCollisionPointRec(mousePos, { slotX, slotY, _slot_size, _slot_size });

            std::shared_ptr<Item::Item> item = (i < chestBackpack.getItems().size()) ? chestBackpack.getItems()[i] : nullptr;

            drawSlot(slotX, slotY, isHovered, item);

            if (isHovered && item != nullptr) {
                itemTooltip = item;
                tooltipPos = { mousePos.x + 15, mousePos.y + 15 };
            }
        }

        if (itemTooltip != nullptr) {
            drawTooltip(font, itemTooltip, tooltipPos.x, tooltipPos.y);
        }
	}

    void ChestUI::drawTooltip(const Font& font, const std::shared_ptr<Item::Item>& item, float x, float y) const {
        const float _font_size = Core::GlobalScaling::scaled(FONT_SIZE);

        const char* _item_name = item->getName().c_str();

        Vector2 textSize = MeasureTextEx(font, _item_name, _font_size, 1.0f);
        float padding = 8.0f;

        DrawRectangle(x, y, textSize.x + (padding * 2), textSize.y + (padding * 2), Fade(BLACK, 0.9f));
        DrawRectangleLines(x, y, textSize.x + (padding * 2), textSize.y + (padding * 2), WHITE);

        DrawTextEx(font, _item_name, { x + padding, y + padding }, _font_size, 1.0f, YELLOW);
    }

    void ChestUI::drawSlot(float x, float y, bool isHovered, const std::shared_ptr<Item::Item>& item) const {
        const float _slot_padding = Core::GlobalScaling::scaled(SLOT_PADDING);
        const float _slot_size = Core::GlobalScaling::scaled(SLOT_SIZE);
        const float _font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const float _padding = Core::GlobalScaling::scaled(PADDING);

        // slot background
        Color slotColor = isHovered ? LIGHTGRAY : DARKGRAY;
        DrawRectangle(x, y, _slot_size, _slot_size, slotColor);
        DrawRectangleLines(x, y, _slot_size, _slot_size, WHITE);

        if (item != nullptr) {
            Texture2D icon = item->getIcon();

            // check if texture is loaded
            if (icon.id > 0) {
                Rectangle source = { 0.0f, 0.0f, (float)icon.width, (float)icon.height };

                Rectangle dest = {
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

        Vector2 mousePos = GetMousePosition();

        float backpackX = _inv_start_x + _text_padding_left;
        float backpackY = _inv_start_y + _text_padding_top + 50;

        for (int i = 0; i < 20; ++i) {
            int col = i % COLS;
            int row = i / COLS;
            float slotX = backpackX + (col * (_slot_size + Core::GlobalScaling::scaled(10.0f)));
            float slotY = backpackY + (row * (_slot_size + Core::GlobalScaling::scaled(10.0f)));

            Rectangle slotRect = { slotX, slotY, _slot_size, _slot_size };

            if (CheckCollisionPointRec(mousePos, slotRect)) {
                return i;
            }
        }

        return -1;
    }

}