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

        // Wymiary i fonty dialogu nadpisania zapisu - jeden zestaw stalych,
        // z ktorych liczymy zarowno pozycje tekstu jak i przyciskow.
        constexpr float MODAL_CONFIRM_FONT_SIZE = 34.0f;
        constexpr float MODAL_BUTTON_WIDTH = 180.0f;
        constexpr float MODAL_BUTTON_HEIGHT = 64.0f;
        constexpr float MODAL_BUTTON_SPACING = 24.0f;
        constexpr float MODAL_TEXT_BUTTON_GAP = 32.0f;

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
        return UIHandler::getCenteredBackButtonRect(0.65f, 0.85f);
    }

    float SaveSlotMenu::getModalGroupTopY() {
        // Tekst i para przyciskow ulozone pionowo i wycentrowane razem na ekranie.
        const float text_height = Core::GlobalScaling::scaled(MODAL_CONFIRM_FONT_SIZE);
        const float button_height = Core::GlobalScaling::scaled(MODAL_BUTTON_HEIGHT);
        const float gap = Core::GlobalScaling::scaled(MODAL_TEXT_BUTTON_GAP);
        const float group_height = text_height + gap + button_height;
        return (static_cast<float>(GetScreenHeight()) - group_height) * 0.5f;
    }

    Rectangle SaveSlotMenu::getModalButtonRect(const int index) {
        const float button_width = Core::GlobalScaling::scaled(MODAL_BUTTON_WIDTH);
        const float button_height = Core::GlobalScaling::scaled(MODAL_BUTTON_HEIGHT);
        const float spacing = Core::GlobalScaling::scaled(MODAL_BUTTON_SPACING);
        const float text_height = Core::GlobalScaling::scaled(MODAL_CONFIRM_FONT_SIZE);
        const float gap = Core::GlobalScaling::scaled(MODAL_TEXT_BUTTON_GAP);

        const float total_width = button_width * 2.0f + spacing;
        const float start_x = (static_cast<float>(GetScreenWidth()) - total_width) * 0.5f;
        const float y = getModalGroupTopY() + text_height + gap;
        return {start_x + index * (button_width + spacing), y, button_width, button_height};
    }

    void SaveSlotMenu::drawSlotCard(const Rectangle& rect, const Game::SaveSlotInfo& slot, const bool hovered, const Font& font) const {
        DrawRectangleRec(rect, hovered ? COLOR_ACCENT_SOFT : Fade(WHITE, 0.10f));
        DrawRectangleLinesEx(rect, Core::GlobalScaling::scaled(2.0f), hovered ? COLOR_ACCENT : Fade(WHITE, 0.4f));

        const float font_spacing = Core::GlobalScaling::scaled(1.0f);
        const float subtitle_font = Core::GlobalScaling::scaled(FONT_SIZE_SUBTITLE);
        const float text_font = Core::GlobalScaling::scaled(FONT_SIZE_TEXT);
        const float body_x = rect.x + Core::GlobalScaling::scaled(20.0f);

        const std::string slot_title = "ZAPIS " + std::to_string(slot.slot);
        drawCenteredText(font, slot_title.c_str(), {rect.x, rect.y + Core::GlobalScaling::scaled(20.0f), rect.width, subtitle_font}, subtitle_font, font_spacing, WHITE);

        const float separator_y = rect.y + Core::GlobalScaling::scaled(75.0f);
        DrawLineEx({body_x, separator_y}, {rect.x + rect.width - Core::GlobalScaling::scaled(20.0f), separator_y},
            1.0f, Fade(WHITE, 0.3f));

        float current_y = separator_y + Core::GlobalScaling::scaled(15.0f);
        const auto drawLine = [&](const char* text, Color color, float extra_gap = 4.0f) {
            DrawTextEx(font, text, {body_x, current_y}, text_font, font_spacing, color);
            current_y += text_font + Core::GlobalScaling::scaled(extra_gap);
        };
        const auto drawLabelValue = [&](const char* label, const std::string& value) {
            drawLine(label, COLOR_ACCENT);
            drawLine(value.c_str(), Fade(WHITE, 0.85f), 10.0f);
        };

        if (!slot.occupied) {
            drawLine("PUSTY SLOT", COLOR_ACCENT, 8.0f);
            drawLine(emptySlotHintForMode(_mode), Fade(WHITE, 0.8f));
            return;
        }

        drawLabelValue("Zapisano:", slot.saved_at);
        drawLabelValue("Poziom:", slot.current_level);
        if (!slot.current_location.empty())
            drawLabelValue("Lokacja:", slot.current_location);
    }

    void SaveSlotMenu::render(const UIHandler& ui) const {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());

        // Lekkie zaciemnienie tla - z menu glownego prawie niewidoczne, a wchodzac
        // z pauzy delikatnie wycisza widok gry pod menu slotow.
        DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, BACKDROP_ALPHA));

        const float font_spacing = Core::GlobalScaling::scaled(2.0f);
        const float title_font_size = Core::GlobalScaling::scaled(FONT_SIZE_TITLE);
        const char* menu_title = titleForMode(_mode);
        drawCenteredText(ui.getFont(), menu_title, {0.0f, Core::GlobalScaling::scaled(60.0f), screen_width, title_font_size}, title_font_size, font_spacing, COLOR_ACCENT);

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
        const float confirm_font = Core::GlobalScaling::scaled(MODAL_CONFIRM_FONT_SIZE);
        drawCenteredText(ui.getFont(), confirm_text, {0.0f, getModalGroupTopY(), screen_width, confirm_font}, confirm_font, font_spacing, WHITE);

        const char* modal_labels[] = {"TAK", "NIE"};
        for (int i = 0; i < 2; ++i) {
            const Rectangle r = getModalButtonRect(i);
            ui.drawMenuButton(r, modal_labels[i], CheckCollisionPointRec(mouse_position, r) ? 1.0f : 0.0f);
        }
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
