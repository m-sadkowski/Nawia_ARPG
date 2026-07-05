#include "UIHandler.h"

#include <GlobalScaling.h>
#include <SaveGameManager.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <string>

namespace Nawia::UI
{
    namespace
    {
        const char* const AUTHOR_NAMES[] = {AUTHOR_NAME_1, AUTHOR_NAME_2, AUTHOR_NAME_3, AUTHOR_NAME_4};
        const char* const ACTOR_NAMES[] = {
            "Szymon \"Logos\" Kulasiewicz - Logos",
            "Jakub \"Gruby\" Kulesza - Gzib",
            "Anastazja \"Babka\" Neczajewska - Szeptucha & Ocalona",
            "Jan \"Roki\" Ciupa - Soltys",
            "Sara \"Kierownik\" Rzoska - Wiedzma",
            "Julia \"Julka\" Morawska - Zagubiona",
            "Pawel \"Pawko\" Kondratowicz - Zielarz",
            "Kalina \"Smyk\" Cheba - Siostra Mileny"
        };

        std::vector<Rectangle> getVerticalMenuLayout(const int button_count, const bool centered = false)
        {
            const float screen_width = static_cast<float>(GetScreenWidth());
            const float screen_height = static_cast<float>(GetScreenHeight());
            const float scaled_width = Core::GlobalScaling::scaled(BUTTON_WIDTH);
            const float scaled_height = Core::GlobalScaling::scaled(BUTTON_HEIGHT);
            const float scaled_spacing = Core::GlobalScaling::scaled(BUTTON_SPACING);
            const float start_x = centered ? (screen_width - scaled_width) / 2.0f : screen_width * MENU_SIDE_X_PCT;
            const float total_menu_height = button_count * scaled_height + (button_count - 1) * scaled_spacing;
            const float start_y = centered
                ? (screen_height - total_menu_height) / 2.0f + Core::GlobalScaling::scaled(40.0f)
                : screen_height * MENU_START_Y_PCT;

            std::vector<Rectangle> button_rectangles;
            button_rectangles.reserve(button_count);
            for (int i = 0; i < button_count; ++i)
                button_rectangles.push_back({start_x, start_y + i * (scaled_height + scaled_spacing), scaled_width, scaled_height});

            return button_rectangles;
        }

        Rectangle getAuthorsBackButtonRect()
        {
            const float screen_height = static_cast<float>(GetScreenHeight());
            Rectangle rect = UIHandler::getCenteredBackButtonRect();
            rect.y = std::min(
                screen_height - rect.height - screen_height * 0.035f,
                rect.y + screen_height * 0.08f);
            return rect;
        }

        void drawCenteredText(
            const Font& font,
            const char* text,
            const float y,
            const float font_size,
            const float spacing,
            const Color color)
        {
            const float screen_width = static_cast<float>(GetScreenWidth());
            const Vector2 text_size = MeasureTextEx(font, text, font_size, spacing);
            DrawTextEx(font, text, {(screen_width - text_size.x) * 0.5f, y}, font_size, spacing, color);
        }

        void drawParticlesFx(const float width, const float height, const float time)
        {
            for (int i = 0; i < SMOKE_LAYER_COUNT; ++i)
            {
                const float seed = static_cast<float>(i) * 11.73f + 3.1f;
                const float travel = fract(hash01(seed) + time * (0.012f + hash01(seed + 2.0f) * 0.016f));
                const float pos_x = width * (0.05f + hash01(seed + 1.0f) * 0.90f) + std::sin(time * (0.22f + hash01(seed + 4.0f) * 0.18f) + seed) * width * 0.06f;
                const float pos_y = height * (1.12f - travel * 1.24f);
                const float radius = Core::GlobalScaling::scaled(110.0f + hash01(seed + 5.0f) * 150.0f);
                const float alpha = (0.35f + (1.0f - travel) * 0.65f) * (0.035f + hash01(seed + 6.0f) * 0.07f);
                DrawCircleGradient(static_cast<int>(pos_x), static_cast<int>(pos_y), radius, withAlpha(LIGHTGRAY, alpha), withAlpha(DARKGRAY, 0.0f));
            }

            for (int i = 0; i < FIRE_PARTICLE_COUNT; ++i)
            {
                const float seed = static_cast<float>(i) * 17.13f + 8.0f;
                const float cycle = fract(hash01(seed) + time * (0.10f + hash01(seed + 1.0f) * 0.22f));
                const float rise = 1.0f - cycle;
                const float pos_x = width * (0.03f + hash01(seed + 2.0f) * 0.94f) + std::sin(time * (1.0f + hash01(seed + 3.0f) * 1.5f) + seed) * width * (0.01f + hash01(seed + 9.0f) * 0.02f);
                const float pos_y = height * (1.04f - rise * 1.18f);
                const float radius = Core::GlobalScaling::scaled(1.5f + hash01(seed + 7.0f) * hash01(seed + 7.0f) * 12.0f) * (0.45f + rise * 0.95f);
                const float alpha = (0.10f + rise * 0.50f) * (0.55f + hash01(seed + 6.0f) * 0.45f);
                DrawCircleGradient(static_cast<int>(pos_x), static_cast<int>(pos_y), radius, withAlpha(COLOR_GOLDEN_TEXT, alpha), withAlpha(COLOR_SLAVIC_ORANGE, alpha * 0.35f));
            }
        }

        std::vector<MenuButtonDef> buildPauseMenuButtons(const bool saves_enabled)
        {
            std::vector<MenuButtonDef> buttons;
            buttons.push_back({LABEL_CONTINUE, MenuAction::Play});
            if (saves_enabled)
                buttons.push_back({LABEL_SAVE_GAME, MenuAction::SaveGame});
            buttons.push_back({LABEL_LOAD_GAME, MenuAction::LoadGame});
            buttons.push_back({LABEL_SETTINGS, MenuAction::Settings});
            buttons.push_back({LABEL_MAIN_MENU, MenuAction::MainMenu});
            return buttons;
        }
    }

    Rectangle UIHandler::getCenteredBackButtonRect(const float width_factor, const float height_factor)
    {
        const float button_width = Core::GlobalScaling::scaled(BUTTON_WIDTH * width_factor);
        const float button_height = Core::GlobalScaling::scaled(BUTTON_HEIGHT * height_factor);
        const float bottom_offset = Core::GlobalScaling::scaled(BACK_BUTTON_BOTTOM_OFFSET);
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        return {(screen_width - button_width) / 2.0f, screen_height - bottom_offset - button_height, button_width, button_height};
    }

    void UIHandler::updateMenuHoverTimers(const float delta_time)
    {
        if (_is_authors_open)
            updateHoverTimers(delta_time, {getAuthorsBackButtonRect()});
        else if (!_settings_menu && !_level_select_menu && !_save_slot_menu)
            updateHoverTimers(delta_time, getMainMenuLayout(static_cast<int>(buildMainMenuButtons().size())));
    }

    void UIHandler::updateHoverTimers(const float delta_time, const std::vector<Rectangle>& button_rectangles)
    {
        if (_hover_timers.size() != button_rectangles.size())
            _hover_timers.assign(button_rectangles.size(), 0.0f);

        const Vector2 mouse_position = GetMousePosition();
        for (size_t i = 0; i < button_rectangles.size(); ++i)
        {
            if (CheckCollisionPointRec(mouse_position, button_rectangles[i]))
                _hover_timers[i] = std::min(1.0f, _hover_timers[i] + delta_time * 6.0f);
            else
                _hover_timers[i] = std::max(0.0f, _hover_timers[i] - delta_time * 4.0f);
        }
    }

    void UIHandler::drawMenuButtonsStack(const std::vector<MenuButtonDef>& buttons, const std::vector<Rectangle>& rectangles) const
    {
        const Vector2 mouse_position = GetMousePosition();
        for (size_t i = 0; i < buttons.size(); ++i)
        {
            const float hover_progress = CheckCollisionPointRec(mouse_position, rectangles[i]) ? 1.0f : 0.0f;
            drawMenuButton(rectangles[i], buttons[i].label, hover_progress);
        }
    }

    int UIHandler::getClickedButtonIndex(const std::vector<Rectangle>& rectangles) const
    {
        if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            return -1;

        const Vector2 mouse_position = GetMousePosition();
        for (size_t i = 0; i < rectangles.size(); ++i)
        {
            if (CheckCollisionPointRec(mouse_position, rectangles[i]))
                return static_cast<int>(i);
        }
        return -1;
    }

    void UIHandler::renderVerticalMenu(const char* title, const std::vector<MenuButtonDef>& buttons, const bool centered) const
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float font_spacing = Core::GlobalScaling::scaled(2.0f);
        const float title_font_size = Core::GlobalScaling::scaled(centered ? FONT_SIZE_TITLE : FONT_SIZE_MAIN_TITLE);
        const Vector2 title_size = MeasureTextEx(_font, title, title_font_size, font_spacing);
        const float button_width = Core::GlobalScaling::scaled(BUTTON_WIDTH);
        const Vector2 title_position = centered
            ? Vector2{(screen_width - title_size.x) / 2.0f, Core::GlobalScaling::scaled(80.0f)}
            : Vector2{(screen_width * MENU_SIDE_X_PCT) + (button_width - title_size.x) / 2.0f, screen_height * 0.22f + std::sin(static_cast<float>(GetTime()) * 0.8f) * Core::GlobalScaling::scaled(4.0f)};

        if (!centered)
        {
            DrawTextEx(_font, title, {title_position.x + 6, title_position.y + 6}, title_font_size, font_spacing, withAlpha(BLACK, 0.8f));
            DrawTextEx(_font, title, title_position, title_font_size, font_spacing, WHITE);
        }
        else
        {
            DrawTextEx(_font, title, title_position, title_font_size, font_spacing, COLOR_ACCENT);
        }

        const auto button_rectangles = getVerticalMenuLayout(static_cast<int>(buttons.size()), centered);
        drawMenuButtonsStack(buttons, button_rectangles);
    }

    std::vector<MenuButtonDef> UIHandler::buildMainMenuButtons() const
    {
        std::vector<MenuButtonDef> buttons;
        buttons.push_back({LABEL_NEW_GAME, MenuAction::NewGame});

        if (_save_game_manager && _save_game_manager->hasAnySave())
        {
            buttons.push_back({LABEL_CONTINUE, MenuAction::ContinueGame});
            buttons.push_back({LABEL_LOAD_GAME, MenuAction::LoadGame});
        }

        buttons.push_back({LABEL_SETTINGS, MenuAction::Settings});
        buttons.push_back({LABEL_AUTHORS, MenuAction::Authors});
        buttons.push_back({LABEL_EXIT, MenuAction::Exit});
        return buttons;
    }

    std::vector<Rectangle> UIHandler::getMainMenuLayout(const int button_count)
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float scaled_width = Core::GlobalScaling::scaled(BUTTON_WIDTH);
        const float scaled_height = Core::GlobalScaling::scaled(BUTTON_HEIGHT);
        const float scaled_spacing = Core::GlobalScaling::scaled(BUTTON_SPACING);
        const float start_x = screen_width * MENU_SIDE_X_PCT;
        const float total_height = button_count * scaled_height + std::max(0, button_count - 1) * scaled_spacing;
        const float top_margin = Core::GlobalScaling::scaled(80.0f);
        const float available_height = screen_height - top_margin - Core::GlobalScaling::scaled(60.0f);
        const float start_y = (total_height < available_height)
            ? top_margin + (available_height - total_height) * 0.5f
            : top_margin;

        std::vector<Rectangle> rectangles;
        rectangles.reserve(button_count);
        for (int i = 0; i < button_count; ++i)
            rectangles.push_back({start_x, start_y + i * (scaled_height + scaled_spacing), scaled_width, scaled_height});

        return rectangles;
    }

    void UIHandler::renderMainMenuTitle() const
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float font_spacing = Core::GlobalScaling::scaled(2.0f);
        const float title_font_size = Core::GlobalScaling::scaled(FONT_SIZE_MAIN_TITLE);
        const char* title_text = "Nawia";
        const Vector2 title_size = MeasureTextEx(_font, title_text, title_font_size, font_spacing);
        const float side_margin = screen_width * MENU_SIDE_X_PCT;
        const float title_x = screen_width - side_margin - title_size.x;
        const float title_y = (screen_height - title_size.y) * 0.5f + std::sin(static_cast<float>(GetTime()) * 0.8f) * Core::GlobalScaling::scaled(4.0f);

        DrawTextEx(_font, title_text, {title_x + 6.0f, title_y + 6.0f}, title_font_size, font_spacing, withAlpha(BLACK, 0.8f));
        DrawTextEx(_font, title_text, {title_x, title_y}, title_font_size, font_spacing, WHITE);
    }

    void UIHandler::renderMainMenu() const
    {
        if (_is_authors_open)
        {
            renderAuthorsMenu();
            return;
        }

        drawSharedMenuBackground();
        renderMainMenuTitle();

        const auto buttons = buildMainMenuButtons();
        drawMenuButtonsStack(buttons, getMainMenuLayout(static_cast<int>(buttons.size())));
    }

    void UIHandler::renderAuthorsMenu() const
    {
        drawSharedMenuBackground();
        const float screen_height = static_cast<float>(GetScreenHeight());
        const Rectangle back_button_rect = getAuthorsBackButtonRect();
        const float available_top = screen_height * 0.055f;
        const float available_bottom = back_button_rect.y - screen_height * 0.035f;
        const float available_height = std::max(260.0f, available_bottom - available_top);
        const float title_font_size = std::clamp(available_height * 0.075f, 28.0f, 58.0f);
        const float heading_font_size = std::clamp(available_height * 0.056f, 23.0f, 42.0f);
        const float entry_font_size = std::clamp(available_height * 0.050f, 22.0f, 38.0f);
        const float author_gap = available_height * 0.012f;
        const float section_gap = available_height * 0.035f;
        const float actor_gap = available_height * 0.007f;

        float current_y = available_top;
        drawCenteredText(_font, "Autorzy:", current_y, title_font_size, 2.0f, COLOR_ACCENT);
        current_y += title_font_size + author_gap;

        for (const auto* name : AUTHOR_NAMES)
        {
            drawCenteredText(_font, name, current_y, entry_font_size, 2.0f, WHITE);
            current_y += entry_font_size + author_gap;
        }

        current_y += section_gap;
        drawCenteredText(_font, "Aktorzy (w kolejnosci wystepowania):", current_y, heading_font_size, 2.0f, COLOR_ACCENT);
        current_y += heading_font_size + actor_gap * 2.0f;

        for (const auto* name : ACTOR_NAMES)
        {
            drawCenteredText(_font, name, current_y, entry_font_size, 1.0f, WHITE);
            current_y += entry_font_size + actor_gap;
        }

        drawMenuButton(back_button_rect, LABEL_BACK, CheckCollisionPointRec(GetMousePosition(), back_button_rect) ? 1.0f : 0.0f);
    }

    MenuAction UIHandler::handleMenuInput()
    {
        if (_is_authors_open)
        {
            if (getClickedButtonIndex({getAuthorsBackButtonRect()}) == 0)
            {
                _is_authors_open = false;
                return MenuAction::None;
            }
        }
        else
        {
            const auto buttons = buildMainMenuButtons();
            const int clicked_index = getClickedButtonIndex(getMainMenuLayout(static_cast<int>(buttons.size())));
            if (clicked_index >= 0)
                return buttons[static_cast<size_t>(clicked_index)].action;
        }

        if (IsKeyPressed(KEY_ESCAPE) && _is_authors_open)
            _is_authors_open = false;

        return MenuAction::None;
    }

    void UIHandler::drawMenuButton(const Rectangle& rectangle, const char* text, const float hover_progress) const
    {
        static std::map<std::string, float> button_hover_progress;

        const std::string button_key = TextFormat("%s:%.0f:%.0f:%.0f:%.0f", text, rectangle.x, rectangle.y, rectangle.width, rectangle.height);
        const float target_hover = hover_progress > 0.01f ? 1.0f : 0.0f;
        float& visual_hover = button_hover_progress[button_key];
        const float animation_speed = target_hover > visual_hover ? 9.0f : 6.0f;
        visual_hover += (target_hover - visual_hover) * std::min(1.0f, GetFrameTime() * animation_speed);

        if (_menu_btn_idle && _menu_btn_idle->id > 0)
        {
            DrawTexturePro(
                *_menu_btn_idle,
                {0.0f, 0.0f, static_cast<float>(_menu_btn_idle->width), static_cast<float>(_menu_btn_idle->height)},
                rectangle,
                {0.0f, 0.0f},
                0.0f,
                WHITE);
        }
        else
        {
            DrawRectangleRec(rectangle, withAlpha(COLOR_PANEL_BG, 0.45f + visual_hover * 0.35f));
            DrawRectangleLinesEx(rectangle, Core::GlobalScaling::scaled(2.0f), LerpColor(withAlpha(WHITE, 0.35f), withAlpha(COLOR_ACCENT, 0.9f), visual_hover));
            if (visual_hover > 0.01f)
                DrawRectangleGradientV(static_cast<int>(rectangle.x), static_cast<int>(rectangle.y), static_cast<int>(rectangle.width), static_cast<int>(rectangle.height), withAlpha(WHITE, 0.06f * visual_hover), withAlpha(WHITE, 0.0f));
        }

        const float button_font_size = Core::GlobalScaling::scaled(FONT_SIZE_BUTTON);
        const Vector2 text_size = MeasureTextEx(_font, text, button_font_size, 2.0f);
        const Vector2 text_position = {
            rectangle.x + (rectangle.width - text_size.x) / 2.0f + visual_hover * Core::GlobalScaling::scaled(12.0f),
            rectangle.y + (rectangle.height - text_size.y) / 2.0f
        };

        DrawTextEx(_font, text, {text_position.x + 2, text_position.y + 2}, button_font_size, 2.0f, withAlpha(BLACK, 0.5f));
        DrawTextEx(_font, text, text_position, button_font_size, 2.0f, LerpColor(withAlpha(WHITE, 0.95f), WHITE, visual_hover));
    }

    void UIHandler::drawSharedMenuBackground() const
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float current_time = static_cast<float>(GetTime());

        if (_main_menu_background)
        {
            float source_width = static_cast<float>(_main_menu_background->width);
            float source_height = static_cast<float>(_main_menu_background->height);
            const float screen_aspect_ratio = screen_width / screen_height;
            const float texture_aspect_ratio = source_width / source_height;

            if (texture_aspect_ratio > screen_aspect_ratio)
                source_width = source_height * screen_aspect_ratio;
            else
                source_height = source_width / screen_aspect_ratio;

            const float zoom_factor = 0.11f + 0.02f * std::sin(current_time * 0.23f);
            source_width *= (1.0f - zoom_factor);
            source_height *= (1.0f - zoom_factor);

            const float offset_x = std::max(0.0f, (_main_menu_background->width - source_width) * 0.5f) * std::sin(current_time * 0.12f + 0.8f);
            const float offset_y = std::max(0.0f, (_main_menu_background->height - source_height) * 0.5f) * std::cos(current_time * 0.09f - 0.35f);
            const Rectangle source_rectangle = {
                (_main_menu_background->width - source_width) * 0.5f + offset_x,
                (_main_menu_background->height - source_height) * 0.5f + offset_y,
                source_width,
                source_height
            };
            DrawTexturePro(*_main_menu_background, source_rectangle, {0, 0, screen_width, screen_height}, {0, 0}, 0.0f, WHITE);
        }
        else
        {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f));
        }

        DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), withAlpha({30, 14, 10, 255}, 0.10f), withAlpha({5, 5, 8, 255}, 0.48f));
        drawParticlesFx(screen_width, screen_height, current_time);
        DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), withAlpha(COLOR_ACCENT, 0.02f), withAlpha(BLACK, 0.12f));
    }

    void UIHandler::renderPauseMenu(const bool saves_enabled) const
    {
        drawSharedMenuBackground();
        renderVerticalMenu(LABEL_PAUSE, buildPauseMenuButtons(saves_enabled), true);
    }

    MenuAction UIHandler::handlePauseMenuInput(const bool saves_enabled)
    {
        const auto buttons = buildPauseMenuButtons(saves_enabled);
        const int clicked_index = getClickedButtonIndex(getVerticalMenuLayout(static_cast<int>(buttons.size()), true));
        if (clicked_index < 0)
            return MenuAction::None;

        return buttons[static_cast<size_t>(clicked_index)].action;
    }

    void UIHandler::renderGameOverScreen() const
    {
        drawSharedMenuBackground();
        renderVerticalMenu(LABEL_GAME_OVER, {
            {LABEL_RESPAWN, MenuAction::Respawn},
            {LABEL_EXIT, MenuAction::Exit}
        }, true);
    }

    MenuAction UIHandler::handleGameOverInput()
    {
        const int clicked_index = getClickedButtonIndex(getVerticalMenuLayout(2, true));
        if (clicked_index == 0) return MenuAction::Respawn;
        if (clicked_index == 1) return MenuAction::Exit;
        return MenuAction::None;
    }
} // namespace Nawia::UI
