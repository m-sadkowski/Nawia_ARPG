#pragma once

#include <memory>
#include <vector>
#include <raylib.h>

#include "InventoryUI.h"
#include "ChestUI.h"

namespace Nawia::Entity {
    class Player;
    class Entity;
    class Chest;
}

namespace Nawia::Core {
    class EntityManager;
    struct Camera;
    class Settings;
    class ResourceManager;
}

namespace Nawia::UI {

    class StatsUI;
    class SettingsMenu;

    /**
     * @enum MenuAction
     * @brief Actions that can result from menu input handling.
     */
    enum class MenuAction {
        None,       ///< No action taken
        Play,       ///< Start/resume game
        Settings,   ///< Open settings menu
        Exit        ///< Exit game
    };

    /**
     * @class UIHandler
     * @brief Manages all UI rendering: HUD, main menu, and settings menu.
     */
    class UIHandler {
    public:
        UIHandler();
        ~UIHandler();

        void initialize(const std::shared_ptr<Entity::Player>& player, Core::EntityManager* entity_manager, Core::ResourceManager& _resource_manager);
        
        void update(float dt);
        void render(const Core::Camera& camera) const;
        void renderMainMenu() const;
        void renderSettingsMenu() const;
        
        MenuAction handleMenuInput();
        MenuAction handleSettingsInput();
        MenuAction handlePauseMenuInput();  ///< Handle input for ESC pause menu overlay

        // handle general input, ex open EQ on key
        void handleInput();
        
        /// Render pause menu overlay (semi-transparent)
        void renderPauseMenu() const;
        
        /// Open settings menu with current settings
        void openSettings(const Core::Settings& settings);
        
        /// Check if settings were applied (get new settings via getAppliedSettings)
        [[nodiscard]] bool wereSettingsApplied() const;
        
        /// Get the settings that were applied (valid after wereSettingsApplied returns true)
        [[nodiscard]] const Core::Settings& getAppliedSettings() const;
        
        /// Close the settings menu (call after settings are applied)
        void closeSettingsMenu();

        // inventory
        bool isInventoryOpen() const { return _is_inventory_open; }
        void toggleInventory() { _is_inventory_open = !_is_inventory_open; }

        // chest
        void openChest(std::shared_ptr<Entity::Chest> chest);
        void closeChest();

    private:
        void renderPlayerHealthBar() const;
        void renderPlayerAbilityBar() const;
        void renderEnemyHealthBars(const Core::Camera& camera) const;
        
        void drawBar(float x, float y, float width, float height, float percentage, Color fg_color, Color bg_color) const;
        void drawMenuButton(const Rectangle& rect, const char* text, bool is_hovered) const;

        std::shared_ptr<Entity::Player> _player;
        Core::EntityManager* _entity_manager;
        Font _font;
        
        std::unique_ptr<SettingsMenu> _settings_menu;

        std::unique_ptr<InventoryUI> _inventory_ui;
        bool _is_inventory_open = false;

        std::unique_ptr<ChestUI> _chest_ui;
        std::shared_ptr<Entity::Chest> _current_chest;

        std::unique_ptr<StatsUI> _stats_ui;
        
        // Damage Flash
        int _previous_hp = -1;
        float _damage_flash_timer = 0.0f;
    };

} // namespace Nawia::UI

