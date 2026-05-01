#pragma once

#include "InventoryUI.h"
#include "ChestUI.h"
#include "DialogueUI.h"
#include "QuestUI.h"
#include "UIDefines.h"
#include "UIRenderUtils.h"

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
        None, Play, Settings, Authors, Respawn, Exit
    };

    struct MenuButtonDef {
        const char* label;
        MenuAction action;
    };

    class UIHandler {
    public:
        UIHandler();
        ~UIHandler();

        void initialize(const std::shared_ptr<Entity::Player>& player, Core::EntityManager* entity_manager, Core::ResourceManager& _resource_manager, Game::QuestManager* quest_manager, const Core::Settings* settings);
        
        void update(float dt);
        void render(const Core::GameCamera& camera);
        void renderMainMenu() const;
        void renderSettingsMenu() const;
        
        MenuAction handleMenuInput();
        MenuAction handleSettingsInput();
        MenuAction handlePauseMenuInput();
        MenuAction handleGameOverInput();

        void renderGameOverScreen() const;
        void handleInput();
        void renderPauseMenu() const;

        void renderLevelSelectMenu() const;
        void openLevelSelect(const std::vector<World::LevelInfo>& levels);
        void closeLevelSelect();
        std::string handleLevelSelectInput();
        
        void openSettings(const Core::Settings& settings);
        [[nodiscard]] bool wereSettingsApplied() const;
        [[nodiscard]] const Core::Settings& getAppliedSettings() const;
        void closeSettingsMenu();

        void setLevelManager(World::LevelManager* level_manager) { _level_manager = level_manager; }
        void renderLocationInfo() const;

        [[nodiscard]] bool isInventoryOpen() const { return _is_inventory_open; }
        void toggleInventory() { _is_inventory_open = !_is_inventory_open; }

        [[nodiscard]] bool isQuestUIOpen() const { return _is_quest_ui_open; }
        void toggleQuestUI() { _is_quest_ui_open = !_is_quest_ui_open; }

        void openContainer(Entity::InteractiveClickable* container);
        void closeContainer();

        void openDialogue(const Game::DialogueTree& tree) { _dialogueUI.open(tree); }
        void closeDialogue() { _dialogueUI.close(); }

        void openAuthors() { _is_authors_open = true; }
        void closeAuthors() { _is_authors_open = false; }
        [[nodiscard]] bool isAuthorsOpen() const { return _is_authors_open; }

        void showNotification(const std::string& text, float duration = 2.0f);
        [[nodiscard]] bool isInputBlocked() const;
        [[nodiscard]] bool isMouseOverUI() const;
        bool closeOpenWindows();

        void drawSharedMenuBackground() const;
        void drawMenuButton(const Rectangle& rect, const char* text, float hover_timer) const;
        const Font& getFont() const { return _font; }
        void triggerLocationBanner();
        void onLevelLoaded();

    private:
        void renderPlayerHealthBar() const;
        void renderPlayerAbilityBar() const;
        void renderPlayerExperienceBar() const;
        void renderCombatEntityHealthBars(const Core::GameCamera& camera) const;
        void renderVerticalMenu(const char* title, const std::vector<MenuButtonDef>& buttons, bool centered = false) const;
        void updateHoverTimers(float dt, const std::vector<Rectangle>& button_rects);
        void renderAuthorsMenu() const;
        void drawBar(float x, float y, float width, float height, float percentage, Color fg_color, Color bg_color) const;
        void drawOrb(float center_x, float center_y, float radius, float target_percent, float ghost_percent, float wave_speed, Color fill_bright, Color fill_dark, Color bg_color, const char* text) const;
        
        void draw_menu_buttons_stack(const std::vector<MenuButtonDef>& buttons, const std::vector<Rectangle>& rects) const;
        [[nodiscard]] int get_clicked_button_index(const std::vector<Rectangle>& rects) const;

        std::shared_ptr<Entity::Player> _player;
        Core::EntityManager* _entity_manager;
        World::LevelManager* _level_manager = nullptr;
        Font _font;
        std::shared_ptr<Texture2D> _main_menu_background;
        std::shared_ptr<Texture2D> _menu_btn_idle;
        std::shared_ptr<Texture2D> _menu_btn_hover;
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
        int _previous_hp = -1;
        float _damage_flash_timer = 0.0f;

        struct Notification {
            std::string text;
            float timer;
            float duration;
        };
        std::vector<Notification> _notifications;
        
        const Core::Settings* _settings = nullptr;
        float _visual_hp_percent = 1.0f;
        float _visual_exp_percent = 0.0f;
        float _location_banner_timer = 0.0f;
        std::string _last_location_name;
        bool _ignore_next_dt = false;
    };

} // namespace Nawia::UI
