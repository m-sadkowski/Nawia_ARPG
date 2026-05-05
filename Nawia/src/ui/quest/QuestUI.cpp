#include "QuestUI.h"

#include <GlobalScaling.h>
#include <Quest.h>
#include <QuestManager.h>
#include <ResourceManager.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

#include <sstream>
#include <string>

namespace Nawia::UI
{
    namespace
    {
        constexpr Color QUEST_TEXT = { 48, 34, 22, 255 };
        constexpr Color QUEST_TEXT_MUTED = { 80, 56, 36, 210 };
        constexpr Color QUEST_TITLE = { 100, 46, 24, 255 };
        constexpr Color QUEST_COMPLETED = { 34, 82, 84, 255 };

        /**
         * @brief Wlacza lagodniejsze skalowanie tekstury UI.
         */
        void smoothUiTexture(const std::shared_ptr<Texture2D>& texture)
        {
            if (!texture || texture->id <= 0)
                return;

            GenTextureMipmaps(texture.get());
            SetTextureFilter(*texture, TEXTURE_FILTER_TRILINEAR);
        }

        std::vector<std::string> wrapText(const Font& font, const std::string& text, const float font_size, const float spacing, const float max_width)
        {
            std::vector<std::string> lines;
            std::istringstream words(text);
            std::string word;
            std::string current_line;

            while (words >> word)
            {
                const std::string candidate = current_line.empty() ? word : current_line + " " + word;
                const Vector2 candidate_size = MeasureTextEx(font, candidate.c_str(), font_size, spacing);

                if (candidate_size.x <= max_width || current_line.empty())
                {
                    current_line = candidate;
                    continue;
                }

                lines.push_back(current_line);
                current_line = word;
            }

            if (!current_line.empty())
                lines.push_back(current_line);

            if (lines.empty())
                lines.push_back("");

            return lines;
        }

        float drawWrappedText(const Font& font, const std::string& text, const Vector2 position, const float max_width, const float font_size, const float spacing, const Color color)
        {
            const std::vector<std::string> lines = wrapText(font, text, font_size, spacing, max_width);
            const float line_height = font_size + Core::GlobalScaling::scaled(4.0f);

            for (size_t i = 0; i < lines.size(); ++i)
            {
                DrawTextEx(font, lines[i].c_str(), { position.x, position.y + static_cast<float>(i) * line_height }, font_size, spacing, color);
            }

            return static_cast<float>(lines.size()) * line_height;
        }
    }

    QuestUI::QuestUI() {}

    void QuestUI::loadResources(Core::ResourceManager& resource_manager)
    {
        _background = resource_manager.getTexture("assets/textures/ui/questbook.png");
        smoothUiTexture(_background);
    }

    void QuestUI::handleInput()
    {
        const float menu_width = Core::GlobalScaling::scaled(MENU_WIDTH);
        const float menu_height = Core::GlobalScaling::scaled(MENU_HEIGHT);
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float start_x = screen_width - menu_width - Core::GlobalScaling::scaled(50.0f);
        const float start_y = Core::GlobalScaling::scaled(50.0f);

        const float padding = Core::GlobalScaling::scaled(82.0f);
        const float tab_height = Core::GlobalScaling::scaled(34.0f);
        
        const float active_tab_x = start_x + padding;
        const float completed_tab_x = active_tab_x + Core::GlobalScaling::scaled(180.0f);
        
        const float tab_y = start_y + menu_height + Core::GlobalScaling::scaled(8.0f);
        const Rectangle tab_active_rect = { active_tab_x, tab_y, Core::GlobalScaling::scaled(170.0f), tab_height };
        const Rectangle tab_completed_rect = { completed_tab_x, tab_y, Core::GlobalScaling::scaled(170.0f), tab_height };
        
        const Vector2 mouse_position = GetMousePosition();
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            if (CheckCollisionPointRec(mouse_position, tab_active_rect))
            {
                _current_tab = QuestTab::Active;
                _selected_quest_index = 0;
            }
            else if (CheckCollisionPointRec(mouse_position, tab_completed_rect))
            {
                _current_tab = QuestTab::Completed;
                _selected_quest_index = 0;
            }
            
            // Obsluga klikniecia w liste questow.
            const float list_start_x = start_x + padding;
            const float list_start_y = start_y + Core::GlobalScaling::scaled(94.0f);
            const float list_width = Core::GlobalScaling::scaled(224.0f);
            const float item_height = Core::GlobalScaling::scaled(38.0f);
            
            for (int i = 0; i < 10; ++i)
            {
                const Rectangle item_rect = { list_start_x, list_start_y + i * item_height, list_width, item_height };
                if (CheckCollisionPointRec(mouse_position, item_rect))
                    _selected_quest_index = i;
            }
        }
    }

    void QuestUI::render(const Font& font, Game::QuestManager* quest_manager) const
    {
        if (!quest_manager)
            return;
        
        const float font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const float padding = Core::GlobalScaling::scaled(82.0f);
        const float menu_width = Core::GlobalScaling::scaled(MENU_WIDTH);
        const float menu_height = Core::GlobalScaling::scaled(MENU_HEIGHT);
        
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float start_x = screen_width - menu_width - Core::GlobalScaling::scaled(50.0f);
        const float start_y = Core::GlobalScaling::scaled(50.0f);

        const Rectangle panel_rect = { start_x, start_y, menu_width, menu_height };
        if (_background && _background->id > 0)
        {
            DrawTexturePro(
                *_background,
                { 0.0f, 0.0f, static_cast<float>(_background->width), static_cast<float>(_background->height) },
                panel_rect,
                { 0.0f, 0.0f },
                0.0f,
                WHITE);
        }
        else
        {
            DrawRectangleRec(panel_rect, withAlpha(COLOR_PANEL_BG, 0.98f));
            DrawRectangleLinesEx(panel_rect, 2.0f, withAlpha(COLOR_ACCENT, 0.8f));
        }

        // Zakladki.
        const float tab_height = Core::GlobalScaling::scaled(34.0f);
        const float active_tab_x = start_x + padding;
        const float completed_tab_x = active_tab_x + Core::GlobalScaling::scaled(180.0f);
        
        const float tab_y = start_y + menu_height + Core::GlobalScaling::scaled(8.0f);
        const Rectangle tab_active_rect = { active_tab_x, tab_y, Core::GlobalScaling::scaled(170.0f), tab_height };
        const Rectangle tab_completed_rect = { completed_tab_x, tab_y, Core::GlobalScaling::scaled(170.0f), tab_height };
        
        auto draw_tab = [&](const Rectangle& rect, const char* label, bool active)
        {
            const Color tab_bg = active ? withAlpha(COLOR_ACCENT, 0.24f) : withAlpha(QUEST_TEXT, 0.10f);
            const Color tab_border = active ? withAlpha(QUEST_TITLE, 0.85f) : withAlpha(QUEST_TEXT, 0.25f);
            const Color text_color = active ? QUEST_TITLE : QUEST_TEXT_MUTED;

            DrawRectangleRec(rect, withAlpha(tab_bg, 0.55f));
            DrawRectangleLinesEx(rect, 1.5f, tab_border);
            
            const float tab_font_size = Core::GlobalScaling::scaled(18.0f);
            const Vector2 text_size = MeasureTextEx(font, label, tab_font_size, 1.0f);
            DrawTextEx(font, label, { rect.x + (rect.width - text_size.x) / 2.0f, rect.y + (rect.height - text_size.y) / 2.0f }, tab_font_size, 1.0f, text_color);
        };

        draw_tab(tab_active_rect, "Aktywne", _current_tab == QuestTab::Active);
        draw_tab(tab_completed_rect, "Ukonczone", _current_tab == QuestTab::Completed);

        // Lista questow z aktualnej zakladki.
        std::vector<Game::Quest*> quests = (_current_tab == QuestTab::Active) ? quest_manager->getActiveQuests() : quest_manager->getCompletedQuests();
        
        if (_selected_quest_index >= static_cast<int>(quests.size()))
            _selected_quest_index = quests.empty() ? 0 : static_cast<int>(quests.size()) - 1;
        
        // Lewa kolumna z lista.
        const float list_start_x = start_x + padding;
        const float list_start_y = start_y + Core::GlobalScaling::scaled(94.0f);
        const float list_width = Core::GlobalScaling::scaled(224.0f);
        const float list_height = menu_height - (list_start_y - start_y) - Core::GlobalScaling::scaled(112.0f);
        
        drawQuestList(font, quests, list_start_x, list_start_y, list_width, list_height);
        
        // Prawa kolumna ze szczegolami.
        const float details_start_x = start_x + Core::GlobalScaling::scaled(414.0f);
        const float details_width = Core::GlobalScaling::scaled(224.0f);
        
        if (!quests.empty() && _selected_quest_index >= 0 && _selected_quest_index < static_cast<int>(quests.size()))
            drawQuestDetails(font, quests[_selected_quest_index], details_start_x, list_start_y, details_width, list_height);
        else
            DrawTextEx(font, "Brak zadan w tej zakladce.", { details_start_x, list_start_y }, font_size, 1.0f, QUEST_TEXT_MUTED);
    }

    void QuestUI::drawQuestList(const Font& font, const std::vector<Game::Quest*>& quests, float start_x, float start_y, float width, float height) const
    {
        const float item_height = Core::GlobalScaling::scaled(38.0f);
        const float font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const Vector2 mouse_position = GetMousePosition();
        
        for (int i = 0; i < static_cast<int>(quests.size()); ++i)
        {
            const float current_y = start_y + i * item_height;
            if (current_y + item_height > start_y + height)
                break;
            
            const Rectangle item_rect = { start_x, current_y, width, item_height };
            const bool is_hovered = CheckCollisionPointRec(mouse_position, item_rect);
            
            if (i == _selected_quest_index)
                DrawRectangleRec(item_rect, withAlpha(COLOR_ACCENT, 0.18f));
            else if (is_hovered)
                DrawRectangleRec(item_rect, withAlpha(QUEST_TEXT, 0.08f));
            
            const Color text_color = (i == _selected_quest_index || is_hovered) ? QUEST_TITLE : QUEST_TEXT_MUTED;
            // Maly margines wewnatrz wiersza listy.
            DrawTextEx(font, quests[i]->name.c_str(), { start_x + Core::GlobalScaling::scaled(30.0f), current_y + (item_height - font_size) / 2.0f }, font_size, 1.0f, text_color);
        }
    }

    void QuestUI::drawQuestDetails(const Font& font, Game::Quest* quest, float start_x, float start_y, float width, float height) const
    {
        const float font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const float font_size_large = Core::GlobalScaling::scaled(FONT_SIZE * 1.12f);
        const float padding = Core::GlobalScaling::scaled(10.0f);
        const float text_spacing = Core::GlobalScaling::scaled(1.0f);
        
        float current_y = start_y;
        
        current_y += drawWrappedText(font, quest->name, { start_x, current_y }, width, font_size_large, text_spacing, QUEST_TITLE);
        current_y += padding;
        
        current_y += drawWrappedText(font, quest->description, { start_x, current_y }, width, font_size, text_spacing, QUEST_TEXT);
        current_y += padding;
        
        DrawTextEx(font, "Cele:", { start_x, current_y }, font_size, text_spacing, QUEST_TITLE);
        current_y += font_size + padding;
        
        for (const auto& objective : quest->objectives)
        {
            std::string objective_text = "- " + objective.description;
            if (objective.required_count > 1)
                objective_text += " (" + std::to_string(objective.current_count) + "/" + std::to_string(objective.required_count) + ")";
            
            const Color text_color = objective.isCompleted() ? QUEST_COMPLETED : QUEST_TEXT_MUTED;
            current_y += drawWrappedText(font, objective_text, { start_x + 10.0f, current_y }, width - Core::GlobalScaling::scaled(10.0f), font_size, text_spacing, text_color);
            current_y += padding / 2.0f;
        }
        
        current_y += padding;
        const std::string progress_string = "Postep: " + quest->getProgressString();
        drawWrappedText(font, progress_string, { start_x, current_y }, width, font_size, text_spacing, withAlpha(QUEST_TEXT, 0.55f));
    }

} // namespace Nawia::UI
