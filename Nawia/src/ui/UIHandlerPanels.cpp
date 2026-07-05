#include "UIHandler.h"

#include <GlobalScaling.h>
#include <InteractiveClickable.h>
#include <LevelSelectMenu.h>
#include <QuestUI.h>
#include <SettingsMenu.h>

namespace Nawia::UI
{
    void UIHandler::openSaveSlotMenu(const std::vector<Game::SaveSlotInfo>& slots, const SaveSlotMenu::Mode mode)
    {
        _save_slot_menu = std::make_unique<SaveSlotMenu>(slots, mode);
    }

    void UIHandler::closeSaveSlotMenu()
    {
        _save_slot_menu.reset();
    }

    SaveSlotMenu::Mode UIHandler::getSaveSlotMenuMode() const
    {
        return _save_slot_menu ? _save_slot_menu->getMode() : SaveSlotMenu::Mode::Load;
    }

    void UIHandler::renderSaveSlotMenu() const
    {
        if (!_save_slot_menu)
            return;

        _save_slot_menu->render(*this);
    }

    int UIHandler::handleSaveSlotInput()
    {
        if (!_save_slot_menu)
            return 0;

        return _save_slot_menu->handleInput();
    }

    void UIHandler::renderSettingsMenu() const
    {
        if (!_settings_menu)
            return;

        drawSharedMenuBackground();
        _settings_menu->render(*this);
    }

    MenuAction UIHandler::handleSettingsInput()
    {
        if (!_settings_menu)
            return MenuAction::None;

        if (_settings_menu->handleInput())
        {
            _settings_menu.reset();
            return MenuAction::Play;
        }

        return MenuAction::None;
    }

    void UIHandler::openSettings(const Core::Settings& settings)
    {
        _settings_menu = std::make_unique<SettingsMenu>(settings);
    }

    bool UIHandler::wereSettingsApplied() const
    {
        return _settings_menu && _settings_menu->wasApplied();
    }

    const Core::Settings& UIHandler::getAppliedSettings() const
    {
        return _settings_menu->getSettings();
    }

    void UIHandler::closeSettingsMenu()
    {
        _settings_menu.reset();
    }

    void UIHandler::renderLevelSelectMenu() const
    {
        if (!_level_select_menu)
            return;

        drawSharedMenuBackground();
        _level_select_menu->render(*this);
    }

    void UIHandler::openLevelSelect(const std::vector<World::LevelInfo>& levels)
    {
        _level_select_menu = std::make_unique<LevelSelectMenu>(levels);
    }

    void UIHandler::closeLevelSelect()
    {
        _level_select_menu.reset();
    }

    std::string UIHandler::handleLevelSelectInput()
    {
        if (IsKeyPressed(KEY_ESCAPE))
            return "BACK";

        if (_level_select_menu)
            return _level_select_menu->handleInput();

        return "";
    }

    void UIHandler::openContainer(Entity::InteractiveClickable* container)
    {
        _current_container = container;
        _is_inventory_open = true;
        _is_quest_ui_open = false;
    }

    void UIHandler::closeContainer()
    {
        _current_container = nullptr;
    }

    bool UIHandler::isInputBlocked() const
    {
        return _dialogueUI.isOpen() || _current_container || isMouseOverUI();
    }

    bool UIHandler::isMouseOverUI() const
    {
        const Vector2 mouse_pos = GetMousePosition();

        if (_is_inventory_open)
        {
            const float inv_x = Core::GlobalScaling::scaled(50.0f);
            const float inv_y = Core::GlobalScaling::scaled(50.0f);
            const float inv_width = Core::GlobalScaling::scaled(InventoryUI::INV_WIDTH);
            const float inv_height = Core::GlobalScaling::scaled(InventoryUI::INV_HEIGHT);
            const Rectangle rect = {inv_x, inv_y, inv_width, inv_height};
            if (CheckCollisionPointRec(mouse_pos, rect))
                return true;
        }

        if (_is_quest_ui_open)
        {
            const float menu_width = Core::GlobalScaling::scaled(QuestUI::MENU_WIDTH);
            const float menu_height = Core::GlobalScaling::scaled(QuestUI::MENU_HEIGHT);
            const float screen_width = static_cast<float>(GetScreenWidth());
            const float start_x = screen_width - menu_width - Core::GlobalScaling::scaled(50.0f);
            const float start_y = Core::GlobalScaling::scaled(50.0f);
            const Rectangle rect = {start_x, start_y, menu_width, menu_height + Core::GlobalScaling::scaled(50.0f)};
            if (CheckCollisionPointRec(mouse_pos, rect))
                return true;
        }

        return false;
    }

    bool UIHandler::closeOpenWindows()
    {
        bool closed_anything = false;

        if (_is_inventory_open) {
            _is_inventory_open = false;
            closed_anything = true;
        }
        if (_is_quest_ui_open) {
            _is_quest_ui_open = false;
            closed_anything = true;
        }
        if (_current_container) {
            closeContainer();
            closed_anything = true;
        }
        if (_dialogueUI.isOpen()) {
            closeDialogue();
            closed_anything = true;
        }

        return closed_anything;
    }
} // namespace Nawia::UI
