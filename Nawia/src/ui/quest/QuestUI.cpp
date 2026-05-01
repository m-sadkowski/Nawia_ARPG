#include "QuestUI.h"
#include "UIDefines.h"
#include "UIRenderUtils.h"

#include <QuestManager.h>
#include <Quest.h>
#include <GlobalScaling.h>

namespace Nawia::UI
{

    QuestUI::QuestUI() {}

    void QuestUI::handleInput()
    {
        const float menu_width = Core::GlobalScaling::scaled(MENU_WIDTH);
        const float menu_height = Core::GlobalScaling::scaled(MENU_HEIGHT);
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float start_x = screen_width - menu_width - Core::GlobalScaling::scaled(50.0f);
        const float start_y = Core::GlobalScaling::scaled(50.0f);

        const float padding = Core::GlobalScaling::scaled(25.0f); // Increased base padding
        const float tab_height = Core::GlobalScaling::scaled(40.0f);
        
        const float active_tab_x = start_x + padding;
        const float completed_tab_x = active_tab_x + Core::GlobalScaling::scaled(180.0f);
        
        const Rectangle tab_active_rect = { active_tab_x, start_y + padding, Core::GlobalScaling::scaled(170.0f), tab_height };
        const Rectangle tab_completed_rect = { completed_tab_x, start_y + padding, Core::GlobalScaling::scaled(170.0f), tab_height };
        
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
            
            // Handle clicking on list items
            const float list_start_x = start_x + padding;
            const float list_start_y = start_y + padding * 1.5f + tab_height;
            const float list_width = menu_width / 2.0f - padding * 1.5f;
            const float item_height = Core::GlobalScaling::scaled(40.0f); // Taller items
            
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
        const float padding = Core::GlobalScaling::scaled(25.0f);
        const float menu_width = Core::GlobalScaling::scaled(MENU_WIDTH);
        const float menu_height = Core::GlobalScaling::scaled(MENU_HEIGHT);
        
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        
        const float start_x = screen_width - menu_width - Core::GlobalScaling::scaled(50.0f);
        const float start_y = Core::GlobalScaling::scaled(50.0f);

        // AAA Premium Background
        DrawRectangleRec({ start_x, start_y, menu_width, menu_height }, withAlpha(COLOR_PANEL_BG, 0.98f));
        DrawRectangleLinesEx({ start_x, start_y, menu_width, menu_height }, 2.0f, withAlpha(COLOR_ACCENT, 0.8f));
        DrawRectangleGradientV(static_cast<int>(start_x), static_cast<int>(start_y), static_cast<int>(menu_width), static_cast<int>(menu_height / 6.0f), withAlpha(WHITE, 0.05f), withAlpha(WHITE, 0.0f));

        // Draw Tabs
        const float tab_height = Core::GlobalScaling::scaled(40.0f);
        const float active_tab_x = start_x + padding;
        const float completed_tab_x = active_tab_x + Core::GlobalScaling::scaled(180.0f);
        
        const Rectangle tab_active_rect = { active_tab_x, start_y + padding, Core::GlobalScaling::scaled(170.0f), tab_height };
        const Rectangle tab_completed_rect = { completed_tab_x, start_y + padding, Core::GlobalScaling::scaled(170.0f), tab_height };
        
        auto draw_tab = [&](const Rectangle& rect, const char* label, bool active)
        {
            const Color tab_bg = active ? withAlpha(COLOR_ACCENT, 0.3f) : withAlpha(BLACK, 0.4f);
            const Color tab_border = active ? COLOR_ACCENT : withAlpha(WHITE, 0.3f);
            const Color text_color = active ? COLOR_GOLDEN_TEXT : withAlpha(COLOR_PARCHMENT, 0.7f);

            DrawRectangleRec(rect, tab_bg);
            DrawRectangleLinesEx(rect, 1.5f, tab_border);
            
            const Vector2 text_size = MeasureTextEx(font, label, font_size, 1.0f);
            DrawTextEx(font, label, { rect.x + (rect.width - text_size.x) / 2.0f, rect.y + (rect.height - text_size.y) / 2.0f }, font_size, 1.0f, text_color);
        };

        draw_tab(tab_active_rect, "Aktywne", _current_tab == QuestTab::Active);
        draw_tab(tab_completed_rect, "Ukonczone", _current_tab == QuestTab::Completed);

        // Fetch Quests
        std::vector<Game::Quest*> quests = (_current_tab == QuestTab::Active) ? quest_manager->getActiveQuests() : quest_manager->getCompletedQuests();
        
        if (_selected_quest_index >= static_cast<int>(quests.size()))
            _selected_quest_index = quests.empty() ? 0 : static_cast<int>(quests.size()) - 1;
        
        // List Area
        const float list_start_x = start_x + padding;
        const float list_start_y = start_y + padding * 1.5f + tab_height;
        const float list_width = menu_width / 2.0f - padding * 1.5f;
        const float list_height = menu_height - (list_start_y - start_y) - padding;
        
        drawQuestList(font, quests, list_start_x, list_start_y, list_width, list_height);
        
        // Details Area
        const float separator_x = start_x + menu_width / 2.0f;
        const float details_start_x = separator_x + padding; // Proper padding from separator
        const float details_width = menu_width - (details_start_x - start_x) - padding;
        
        // Draw separator
        DrawLineEx({ separator_x, list_start_y }, { separator_x, start_y + menu_height - padding }, 1.0f, withAlpha(COLOR_ACCENT, 0.2f));
        
        if (!quests.empty() && _selected_quest_index >= 0 && _selected_quest_index < static_cast<int>(quests.size()))
            drawQuestDetails(font, quests[_selected_quest_index], details_start_x, list_start_y, details_width, list_height);
        else
            DrawTextEx(font, "Brak zadan w tej zakladce.", { details_start_x, list_start_y }, font_size, 1.0f, withAlpha(COLOR_PARCHMENT, 0.4f));
    }

    void QuestUI::drawQuestList(const Font& font, const std::vector<Game::Quest*>& quests, float start_x, float start_y, float width, float height) const
    {
        const float item_height = Core::GlobalScaling::scaled(40.0f);
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
                DrawRectangleRec(item_rect, withAlpha(COLOR_ACCENT, 0.15f));
            else if (is_hovered)
                DrawRectangleRec(item_rect, withAlpha(WHITE, 0.05f));
            
            const Color text_color = (i == _selected_quest_index) ? COLOR_GOLDEN_TEXT : (is_hovered ? COLOR_PARCHMENT : withAlpha(COLOR_PARCHMENT, 0.6f));
            // Add small offset for text within the item rectangle
            DrawTextEx(font, quests[i]->name.c_str(), { start_x + 10.0f, current_y + (item_height - font_size) / 2.0f }, font_size, 1.0f, text_color);
        }
    }

    void QuestUI::drawQuestDetails(const Font& font, Game::Quest* quest, float start_x, float start_y, float width, float height) const
    {
        const float font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const float font_size_large = Core::GlobalScaling::scaled(FONT_SIZE * 1.3f);
        const float padding = Core::GlobalScaling::scaled(15.0f);
        
        float current_y = start_y;
        
        // Title
        DrawTextEx(font, quest->name.c_str(), { start_x, current_y }, font_size_large, 1.0f, COLOR_ACCENT);
        current_y += font_size_large + padding;
        
        // Description
        DrawTextEx(font, quest->description.c_str(), { start_x, current_y }, font_size, 1.0f, withAlpha(COLOR_PARCHMENT, 0.8f));
        current_y += Core::GlobalScaling::scaled(80.0f); 
        
        // Objectives
        DrawTextEx(font, "Cele:", { start_x, current_y }, font_size, 1.0f, COLOR_ACCENT); // Made Cele: smaller
        current_y += font_size + padding;
        
        for (const auto& objective : quest->objectives)
        {
            std::string objective_text = "- " + objective.description;
            if (objective.required_count > 1)
                objective_text += " (" + std::to_string(objective.current_count) + "/" + std::to_string(objective.required_count) + ")";
            
            const Color text_color = objective.isCompleted() ? COLOR_SLAVIC_BLUE : withAlpha(COLOR_PARCHMENT, 0.7f);
            DrawTextEx(font, objective_text.c_str(), { start_x + 10.0f, current_y }, font_size, 1.0f, text_color);
            current_y += font_size + padding / 2.0f;
        }
        
        // Progress string
        current_y += padding;
        const std::string progress_string = "Postep: " + quest->getProgressString();
        DrawTextEx(font, progress_string.c_str(), { start_x, current_y }, font_size, 1.0f, withAlpha(COLOR_PARCHMENT, 0.4f));
    }

} // namespace Nawia::UI
