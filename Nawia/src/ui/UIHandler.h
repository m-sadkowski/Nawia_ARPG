#pragma once

#include "InventoryUI.h"
#include "ChestUI.h"
#include "DialogueUI.h"
#include "QuestUI.h"

#include <memory>
#include <vector>
#include <raylib.h>

namespace Nawia::Entity {
    class Player;
    class Entity;
    class InteractiveClickable;
}

namespace Nawia::Core {
    class EntityManager;
    struct GameCamera;
    class Settings;
    class ResourceManager;
}

namespace Nawia::Game {
    class QuestManager;
}

namespace Nawia::World {
    class LevelManager;
    struct LevelInfo;
}

namespace Nawia::UI {

    class StatsUI;
    class SettingsMenu;
    class LevelSelectMenu;

    /**
     * @enum MenuAction
     * @brief Actions that can result from menu input handling.
     */
    enum class MenuAction {
        None,       ///< No action taken
        Play,       ///< Start/resume game
        Settings,   ///< Open settings menu
        Authors,    ///< Open authors/credits
        Respawn,    ///< Respawn at last checkpoint
        Exit        ///< Exit game
    };

    /**
     * @struct MenuButtonDef
     * @brief Definition for a menu button to be rendered by the generic menu system.
     */
    struct MenuButtonDef {
        const char* label;
        MenuAction action;
    };

    /**
     * @class UIHandler
     * @brief Manages all UI rendering: HUD, main menu, and settings menu.
     */
    class UIHandler {
    public:
        UIHandler();
        ~UIHandler();

        void initialize(const std::shared_ptr<Entity::Player>& player, Core::EntityManager* entity_manager, Core::ResourceManager& _resource_manager, Game::QuestManager* quest_manager);
        
        void update(float dt);
        void render(const Core::GameCamera& camera);
        void renderMainMenu() const;
        void renderSettingsMenu() const;
        
        MenuAction handleMenuInput();
        MenuAction handleSettingsInput();
        MenuAction handlePauseMenuInput();  ///< Handle input for ESC pause menu overlay
        MenuAction handleGameOverInput();

        void renderGameOverScreen() const;

        // handle general input, ex open EQ on key
        void handleInput();
        
        /// Render pause menu overlay (semi-transparent)
        void renderPauseMenu() const;

        void renderLevelSelectMenu() const;
        void openLevelSelect(const std::vector<World::LevelInfo>& levels);
        void closeLevelSelect();
        std::string handleLevelSelectInput();
        
        /// Open settings menu with current settings
        void openSettings(const Core::Settings& settings);
        
        /// Check if settings were applied (get new settings via getAppliedSettings)
        [[nodiscard]] bool wereSettingsApplied() const;
        
        /// Get the settings that were applied (valid after wereSettingsApplied returns true)
        [[nodiscard]] const Core::Settings& getAppliedSettings() const;
        
        /// Close the settings menu (call after settings are applied)
        void closeSettingsMenu();

        /// Set level manager reference for location HUD
        void setLevelManager(World::LevelManager* level_manager) { _level_manager = level_manager; }

        /// Render current level + location info on HUD
        void renderLocationInfo() const;

        // inventory
        [[nodiscard]] bool isInventoryOpen() const { return _is_inventory_open; }
        void toggleInventory() { _is_inventory_open = !_is_inventory_open; }

        [[nodiscard]] bool isQuestUIOpen() const { return _is_quest_ui_open; }
        void toggleQuestUI() { _is_quest_ui_open = !_is_quest_ui_open; }

        // chest
        void openContainer(Entity::InteractiveClickable* container);
        void closeContainer();

        void openDialogue(const Game::DialogueTree& tree) { _dialogueUI.open(tree); }
        void closeDialogue() { _dialogueUI.close(); }

        void openAuthors() { _is_authors_open = true; }
        void closeAuthors() { _is_authors_open = false; }
        [[nodiscard]] bool isAuthorsOpen() const { return _is_authors_open; }

        void showNotification(const std::string& text, float duration = 2.0f);

        [[nodiscard]] bool isInputBlocked() const;

        // Unified rendering helpers
        void drawSharedMenuBackground() const;
        void drawMenuButton(const Rectangle& rect, const char* text, float hover_timer) const;
        const Font& getFont() const { return _font; }

        // Standardized font sizes
        static constexpr float FONT_SIZE_TITLE = 80.0f;
        static constexpr float FONT_SIZE_SUBTITLE = 40.0f;
        static constexpr float FONT_SIZE_BUTTON = 24.0f;
        static constexpr float FONT_SIZE_TEXT = 18.0f;

    private:
        void renderPlayerHealthBar() const;
        void renderPlayerAbilityBar() const;
        void renderPlayerExperienceBar() const;
        void renderCombatEntityHealthBars(const Core::GameCamera& camera) const;
        
        /// Generic vertical menu renderer
        void renderVerticalMenu(const char* title, const std::vector<MenuButtonDef>& buttons, bool centered = false) const;
        
        /// Updates hover timers for a set of buttons
        void updateHoverTimers(float dt, const std::vector<Rectangle>& button_rects);

        void renderAuthorsMenu() const;
        
        void drawBar(float x, float y, float width, float height, float percentage, Color fg_color, Color bg_color) const;

        std::shared_ptr<Entity::Player> _player;
        Core::EntityManager* _entity_manager;
        World::LevelManager* _level_manager = nullptr;
        Font _font;
        std::shared_ptr<Texture2D> _main_menu_bg;
        std::shared_ptr<Texture2D> _menu_btn_idle;
        std::shared_ptr<Texture2D> _menu_btn_hover;
        
        // Button hover animation timers
        std::vector<float> _hover_timers;
        
        std::unique_ptr<SettingsMenu> _settings_menu;
        std::unique_ptr<LevelSelectMenu> _level_select_menu;

        std::unique_ptr<InventoryUI> _inventory_ui;
        bool _is_inventory_open = false;

        std::unique_ptr<QuestUI> _quest_ui;
        bool _is_quest_ui_open = false;
        bool _is_authors_open = false;
        
        Game::QuestManager* _quest_manager = nullptr;

        std::unique_ptr<ChestUI> _chest_ui;
        Entity::InteractiveClickable* _current_container = nullptr;

        std::unique_ptr<StatsUI> _stats_ui;

        DialogueUI _dialogueUI;
        
        // Damage Flash
        int _previous_hp = -1;
        float _damage_flash_timer = 0.0f;

        // Notifications
        struct Notification 
    	{
            std::string text;
            float timer;
            float duration;
        };
        std::vector<Notification> _notifications;
    };

} // namespace Nawia::UI
