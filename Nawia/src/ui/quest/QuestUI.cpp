#include "QuestUI.h"
#include <QuestManager.h>
#include <Quest.h>
#include <GlobalScaling.h>

namespace Nawia::UI {

    QuestUI::QuestUI() {}

    void QuestUI::handleInput() {
        const float _menu_width = Core::GlobalScaling::scaled(MENU_WIDTH);
        const float _menu_height = Core::GlobalScaling::scaled(MENU_HEIGHT);
        const float screenWidth = static_cast<float>(GetScreenWidth());
        const float screenHeight = static_cast<float>(GetScreenHeight());
        const float startX = (screenWidth - _menu_width) / 2.0f;
        const float startY = (screenHeight - _menu_height) / 2.0f;

        const float _padding = Core::GlobalScaling::scaled(PADDING);
        const float tabHeight = Core::GlobalScaling::scaled(40.0f);
        
        const float activeTabX = startX + _padding;
        const float compTabX = startX + _padding + Core::GlobalScaling::scaled(180.0f);
        
        Rectangle tabActiveRec = { activeTabX, startY + _padding, Core::GlobalScaling::scaled(170.0f), tabHeight };
        Rectangle tabCompRec = { compTabX, startY + _padding, Core::GlobalScaling::scaled(170.0f), tabHeight };
        
        Vector2 mousePos = GetMousePosition();
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mousePos, tabActiveRec)) {
                _currentTab = QuestTab::Active;
                _selectedQuestIndex = 0;
            } else if (CheckCollisionPointRec(mousePos, tabCompRec)) {
                _currentTab = QuestTab::Completed;
                _selectedQuestIndex = 0;
            }
            
            // Handle clicking on list items
            const float listStartX = startX + _padding;
            const float listStartY = startY + _padding * 2 + tabHeight;
            const float listWidth = _menu_width / 2.0f - _padding * 1.5f;
            const float itemHeight = Core::GlobalScaling::scaled(35.0f);
            
            for (int i=0; i<10; ++i) {
                Rectangle itemRec = { listStartX, listStartY + i * itemHeight, listWidth, itemHeight };
                if (CheckCollisionPointRec(mousePos, itemRec)) {
                    _selectedQuestIndex = i;
                }
            }
        }
    }

    void QuestUI::render(const Font& font, Game::QuestManager* questManager) const {
        if (!questManager) return;
        
        const float _font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const float _padding = Core::GlobalScaling::scaled(PADDING);
        const float _menu_width = Core::GlobalScaling::scaled(MENU_WIDTH);
        const float _menu_height = Core::GlobalScaling::scaled(MENU_HEIGHT);
        
        const float screenWidth = static_cast<float>(GetScreenWidth());
        const float screenHeight = static_cast<float>(GetScreenHeight());
        
        const float startX = (screenWidth - _menu_width) / 2.0f;
        const float startY = (screenHeight - _menu_height) / 2.0f;

        // Draw Background
        DrawRectangle(static_cast<int>(startX), static_cast<int>(startY), static_cast<int>(_menu_width), static_cast<int>(_menu_height), Fade(BLACK, 0.9f));
        DrawRectangleLines(static_cast<int>(startX), static_cast<int>(startY), static_cast<int>(_menu_width), static_cast<int>(_menu_height), WHITE);

        // Draw Tabs
        const float tabHeight = Core::GlobalScaling::scaled(40.0f);
        const float activeTabX = startX + _padding;
        const float compTabX = startX + _padding + Core::GlobalScaling::scaled(180.0f);
        
        Rectangle tabActiveRec = { activeTabX, startY + _padding, Core::GlobalScaling::scaled(170.0f), tabHeight };
        Rectangle tabCompRec = { compTabX, startY + _padding, Core::GlobalScaling::scaled(170.0f), tabHeight };
        
        DrawRectangleRec(tabActiveRec, (_currentTab == QuestTab::Active) ? LIGHTGRAY : DARKGRAY);
        DrawRectangleLinesEx(tabActiveRec, Core::GlobalScaling::scaled(2.0f), WHITE);
        DrawTextEx(font, "Aktywne", { activeTabX + _padding, startY + _padding + _padding }, _font_size, 1.0f, (_currentTab == QuestTab::Active) ? BLACK : WHITE);
        
        DrawRectangleRec(tabCompRec, (_currentTab == QuestTab::Completed) ? LIGHTGRAY : DARKGRAY);
        DrawRectangleLinesEx(tabCompRec, Core::GlobalScaling::scaled(2.0f), WHITE);
        DrawTextEx(font, "Ukonczone", { compTabX + _padding, startY + _padding + _padding }, _font_size, 1.0f, (_currentTab == QuestTab::Completed) ? BLACK : WHITE);

        // Fetch Quests
        std::vector<Game::Quest*> quests = (_currentTab == QuestTab::Active) ? questManager->getActiveQuests() : questManager->getCompletedQuests();
        
        if (_selectedQuestIndex >= (int)quests.size()) {
            _selectedQuestIndex = quests.empty() ? 0 : (int)quests.size() - 1;
        }
        
        // List Area
        const float listStartX = startX + _padding;
        const float listStartY = startY + _padding * 2 + tabHeight;
        const float listWidth = _menu_width / 2.0f - _padding * 1.5f;
        const float listHeight = _menu_height - listStartY + startY - _padding;
        
        drawQuestList(font, quests, listStartX, listStartY, listWidth, listHeight);
        
        // Details Area
        const float detailsStartX = startX + _menu_width / 2.0f + _padding * 0.5f;
        const float detailsWidth = _menu_width / 2.0f - _padding * 1.5f;
        
        // Draw separator
        DrawLine(static_cast<int>(startX + _menu_width / 2.0f), static_cast<int>(listStartY), static_cast<int>(startX + _menu_width / 2.0f), static_cast<int>(startY + _menu_height - _padding), WHITE);
        
        if (!quests.empty() && _selectedQuestIndex >= 0 && _selectedQuestIndex < (int)quests.size()) {
            drawQuestDetails(font, quests[_selectedQuestIndex], detailsStartX, listStartY, detailsWidth, listHeight);
        } else {
            DrawTextEx(font, "Brak zadan w tej zakladce.", { detailsStartX + _padding, listStartY + _padding }, _font_size, 1.0f, GRAY);
        }
    }

    void QuestUI::drawQuestList(const Font& font, const std::vector<Game::Quest*>& quests, float startX, float startY, float width, float height) const {
        const float itemHeight = Core::GlobalScaling::scaled(35.0f);
        const float _font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const float _padding = Core::GlobalScaling::scaled(PADDING);
        
        for (int i = 0; i < (int)quests.size(); ++i) {
            float y = startY + i * itemHeight;
            if (y + itemHeight > startY + height) break;
            
            Rectangle itemRec = { startX, y, width, itemHeight };
            
            if (i == _selectedQuestIndex) {
                DrawRectangleRec(itemRec, Fade(LIGHTGRAY, 0.3f));
            } else if (CheckCollisionPointRec(GetMousePosition(), itemRec)) {
                DrawRectangleRec(itemRec, Fade(DARKGRAY, 0.5f));
            }
            
            DrawTextEx(font, quests[i]->name.c_str(), { startX + _padding, y + _padding / 2 }, _font_size, 1.0f, WHITE);
        }
    }

    void QuestUI::drawQuestDetails(const Font& font, Game::Quest* quest, float startX, float startY, float width, float height) const {
        const float _font_size = Core::GlobalScaling::scaled(FONT_SIZE);
        const float _font_size_large = Core::GlobalScaling::scaled(FONT_SIZE * 1.2f);
        const float _padding = Core::GlobalScaling::scaled(PADDING);
        
        float currentY = startY;
        
        // Title
        DrawTextEx(font, quest->name.c_str(), { startX, currentY }, _font_size_large, 1.0f, YELLOW);
        currentY += _font_size_large + _padding;
        
        // Description
        DrawTextEx(font, quest->description.c_str(), { startX, currentY }, _font_size, 1.0f, LIGHTGRAY);
        currentY += Core::GlobalScaling::scaled(80.0f); 
        
        // Objectives
        DrawTextEx(font, "Cele:", { startX, currentY }, _font_size_large, 1.0f, WHITE);
        currentY += _font_size_large + _padding;
        
        for (const auto& obj : quest->objectives) {
            std::string objText = "- " + obj.description;
            if (obj.required_count > 1) {
                objText += " (" + std::to_string(obj.current_count) + "/" + std::to_string(obj.required_count) + ")";
            }
            
            Color col = obj.isCompleted() ? GREEN : WHITE;
            DrawTextEx(font, objText.c_str(), { startX + _padding, currentY }, _font_size, 1.0f, col);
            currentY += _font_size + _padding;
        }
        
        // Progress string
        currentY += _padding;
        std::string progStr = "Postep ogolny: " + quest->getProgressString();
        DrawTextEx(font, progStr.c_str(), { startX, currentY }, _font_size, 1.0f, GRAY);
    }

}
