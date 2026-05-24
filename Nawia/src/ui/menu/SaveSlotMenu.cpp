#include "SaveSlotMenu.h"

#include <GlobalScaling.h>
#include <UIDefines.h>
#include <UIHandler.h>
#include <UIRenderUtils.h>

#include <algorithm>
#include <string>
#include <utility>

namespace Nawia::UI {

    namespace {
        constexpr float CARD_WIDTH = 320.0f;
        // Wysokosc dobrana tak, by pomiescic najwieksza karte (slot zajety z data,
        // poziomem i lokacja). Wszystkie karty maja te sama wysokosc.
        constexpr float CARD_HEIGHT = 300.0f;
        constexpr float CARD_SPACING = 40.0f;
        constexpr float BACKDROP_ALPHA = 0.4f;

        const char* titleForMode(SaveSlotMenu::Mode mode) {
            switch (mode) {
                case SaveSlotMenu::Mode::Save: return "ZAPISZ GRE";
                case SaveSlotMenu::Mode::SelectDefault: return "WYBIERZ SLOT ZAPISU";
                case SaveSlotMenu::Mode::Load:
                default: return "WCZYTAJ GRE";
            }
        }

        const char* emptySlotHintForMode(SaveSlotMenu::Mode mode) {
            switch (mode) {
                case SaveSlotMenu::Mode::Save: return "Kliknij, aby zapisac";
                case SaveSlotMenu::Mode::SelectDefault: return "Kliknij, aby wybrac";
                case SaveSlotMenu::Mode::Load:
                default: return "Brak zapisu";
            }
        }

        int getClickedRectangleIndex(const std::vector<Rectangle>& rectangles) {
            if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                return -1;

            const Vector2 mouse_position = GetMousePosition();
            for (size_t i = 0; i < rectangles.size(); ++i) {
                if (CheckCollisionPointRec(mouse_position, rectangles[i]))
                    return static_cast<int>(i);
            }

            return -1;
        }
    }

    SaveSlotMenu::SaveSlotMenu(std::vector<Game::SaveSlotInfo> slots, const Mode mode)
        : _slots(std::move(slots))
        , _mode(mode)
    {}

    std::vector<Rectangle> SaveSlotMenu::buildCardLayout(const int slot_count) {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());

        const float card_width = Core::GlobalScaling::scaled(CARD_WIDTH);
        const float card_height = Core::GlobalScaling::scaled(CARD_HEIGHT);
        const float card_spacing = Core::GlobalScaling::scaled(CARD_SPACING);

        const float total_width = slot_count * card_width + std::max(0, slot_count - 1) * card_spacing;
        const float start_x = (screen_width - total_width) * 0.5f;
        const float start_y = (screen_height - card_height) * 0.5f;

        std::vector<Rectangle> rectangles;
        rectangles.reserve(slot_count);
        for (int i = 0; i < slot_count; ++i)
            rectangles.push_back({start_x + i * (card_width + card_spacing), start_y, card_width, card_height});

        return rectangles;
    }

    Rectangle SaveSlotMenu::getBackButtonRect() {
        const float button_width = Core::GlobalScaling::scaled(BUTTON_WIDTH * 0.65f);
        const float button_height = Core::GlobalScaling::scaled(BUTTON_HEIGHT * 0.85f);
        const float bottom_offset = Core::GlobalScaling::scaled(BACK_BUTTON_BOTTOM_OFFSET);
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        return {(screen_width - button_width) * 0.5f, screen_height - bottom_offset, button_width, button_height};
    }

    Rectangle SaveSlotMenu::getModalButtonRect(const int index) {
        const float button_width = Core::GlobalScaling::scaled(180.0f);
        const float button_height = Core::GlobalScaling::scaled(64.0f);
        const float spacing = Core::GlobalScaling::scaled(24.0f);
        const float total_width = button_width * 2.0f + spacing;
        const float start_x = (static_cast<float>(GetScreenWidth()) - total_width) * 0.5f;
        const float y = static_cast<float>(GetScreenHeight()) * 0.58f;
        return {start_x + index * (button_width + spacing), y, button_width, button_height};
    }

    void SaveSlotMenu::drawSlotCard(const Rectangle& rect, const Game::SaveSlotInfo& slot, const bool hovered, const Font& font) const {
        DrawRectangleRec(rect, hovered ? COLOR_ACCENT_SOFT : Fade(WHITE, 0.10f));
        DrawRectangleLinesEx(rect, Core::GlobalScaling::scaled(2.0f), hovered ? COLOR_ACCENT : Fade(WHITE, 0.4f));

        const float font_spacing = Core::GlobalScaling::scaled(1.0f);
        const float subtitle_font = Core::GlobalScaling::scaled(FONT_SIZE_SUBTITLE);
        const float text_font = Core::GlobalScaling::scaled(FONT_SIZE_TEXT);

        const std::string slot_title = "ZAPIS " + std::to_string(slot.slot);
        const Vector2 title_size = MeasureTextEx(font, slot_title.c_str(), subtitle_font, font_spacing);
        DrawTextEx(
            font,
            slot_title.c_str(),
            {rect.x + (rect.width - title_size.x) * 0.5f, rect.y + Core::GlobalScaling::scaled(20.0f)},
            subtitle_font,
            font_spacing,
            WHITE);

        const float separator_y = rect.y + Core::GlobalScaling::scaled(75.0f);
        DrawLineEx(
            {rect.x + Core::GlobalScaling::scaled(20.0f), separator_y},
            {rect.x + rect.width - Core::GlobalScaling::scaled(20.0f), separator_y},
            1.0f,
            Fade(WHITE, 0.3f));

        const float body_x = rect.x + Core::GlobalScaling::scaled(20.0f);
        float current_y = separator_y + Core::GlobalScaling::scaled(15.0f);

        if (!slot.occupied) {
            DrawTextEx(font, "PUSTY SLOT", {body_x, current_y}, text_font, font_spacing, COLOR_ACCENT);
            current_y += text_font + Core::GlobalScaling::scaled(8.0f);
            DrawTextEx(font, emptySlotHintForMode(_mode), {body_x, current_y}, text_font, font_spacing, Fade(WHITE, 0.8f));
            return;
        }

        DrawTextEx(font, "Zapisano:", {body_x, current_y}, text_font, font_spacing, COLOR_ACCENT);
        current_y += text_font + Core::GlobalScaling::scaled(4.0f);
        DrawTextEx(font, slot.saved_at.c_str(), {body_x, current_y}, text_font, font_spacing, Fade(WHITE, 0.85f));
        current_y += text_font + Core::GlobalScaling::scaled(10.0f);

        DrawTextEx(font, "Poziom:", {body_x, current_y}, text_font, font_spacing, COLOR_ACCENT);
        current_y += text_font + Core::GlobalScaling::scaled(4.0f);
        DrawTextEx(font, slot.current_level.c_str(), {body_x, current_y}, text_font, font_spacing, Fade(WHITE, 0.85f));

        if (slot.current_location.empty())
            return;

        current_y += text_font + Core::GlobalScaling::scaled(10.0f);
        DrawTextEx(font, "Lokacja:", {body_x, current_y}, text_font, font_spacing, COLOR_ACCENT);
        current_y += text_font + Core::GlobalScaling::scaled(4.0f);
        DrawTextEx(font, slot.current_location.c_str(), {body_x, current_y}, text_font, font_spacing, Fade(WHITE, 0.85f));
    }

    void SaveSlotMenu::render(const UIHandler& ui) const {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());

        // Lekkie zaciemnienie dopasowane do LevelSelectMenu - z menu glownego
        // tlo praktycznie sie nie zmienia, a z pauzy dodaje subtelnego dimm.
        DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, BACKDROP_ALPHA));

        const float font_spacing = Core::GlobalScaling::scaled(2.0f);
        const float title_font_size = Core::GlobalScaling::scaled(FONT_SIZE_TITLE);
        const char* menu_title = titleForMode(_mode);
        const Vector2 title_size = MeasureTextEx(ui.getFont(), menu_title, title_font_size, font_spacing);
        DrawTextEx(
            ui.getFont(),
            menu_title,
            {(screen_width - title_size.x) * 0.5f, Core::GlobalScaling::scaled(60.0f)},
            title_font_size,
            font_spacing,
            COLOR_ACCENT);

        const int slot_count = static_cast<int>(_slots.size());
        const auto cards = buildCardLayout(slot_count);
        const Vector2 mouse_position = GetMousePosition();

        for (int i = 0; i < slot_count; ++i)
            drawSlotCard(cards[static_cast<size_t>(i)], _slots[static_cast<size_t>(i)], CheckCollisionPointRec(mouse_position, cards[static_cast<size_t>(i)]), ui.getFont());

        const Rectangle back_rect = getBackButtonRect();
        ui.drawMenuButton(back_rect, LABEL_BACK, CheckCollisionPointRec(mouse_position, back_rect) ? 1.0f : 0.0f);

        if (_pending_overwrite_slot <= 0)
            return;

        DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, 0.55f));
        const char* confirm_text = TextFormat("NADPISAC ZAPIS %d?", _pending_overwrite_slot);
        const float confirm_font = Core::GlobalScaling::scaled(34.0f);
        const Vector2 confirm_size = MeasureTextEx(ui.getFont(), confirm_text, confirm_font, font_spacing);
        DrawTextEx(
            ui.getFont(),
            confirm_text,
            {(screen_width - confirm_size.x) * 0.5f, screen_height * 0.46f},
            confirm_font,
            font_spacing,
            WHITE);

        const Rectangle yes_rect = getModalButtonRect(0);
        const Rectangle no_rect = getModalButtonRect(1);
        ui.drawMenuButton(yes_rect, "TAK", CheckCollisionPointRec(mouse_position, yes_rect) ? 1.0f : 0.0f);
        ui.drawMenuButton(no_rect, "NIE", CheckCollisionPointRec(mouse_position, no_rect) ? 1.0f : 0.0f);
    }

    int SaveSlotMenu::handleInput() {
        if (_pending_overwrite_slot > 0) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                _pending_overwrite_slot = 0;
                return 0;
            }

            const int clicked_index = getClickedRectangleIndex({getModalButtonRect(0), getModalButtonRect(1)});
            if (clicked_index == 0)
                return _pending_overwrite_slot;
            if (clicked_index == 1)
                _pending_overwrite_slot = 0;

            return 0;
        }

        if (IsKeyPressed(KEY_ESCAPE))
            return -1;

        const Vector2 mouse_position = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse_position, getBackButtonRect()))
            return -1;

        const auto cards = buildCardLayout(static_cast<int>(_slots.size()));
        const int clicked_index = getClickedRectangleIndex(cards);
        if (clicked_index < 0)
            return 0;

        const Game::SaveSlotInfo& clicked_slot = _slots[static_cast<size_t>(clicked_index)];

        if (_mode == Mode::Load && !clicked_slot.occupied)
            return 0;

        if (_mode != Mode::Load && clicked_slot.occupied) {
            _pending_overwrite_slot = clicked_slot.slot;
            return 0;
        }

        return clicked_slot.slot;
    }

} // namespace Nawia::UI
