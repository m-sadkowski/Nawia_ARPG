#pragma once

#include <Camera.h>
#include <ChestUI.h>
#include <DialogueUI.h>
#include <InventoryUI.h>
#include <QuestUI.h>
#include <SaveSlotMenu.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

#include <raylib.h>

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace Nawia::Entity {
    class Player;
    class Entity;
    class InteractiveClickable;
}

namespace Nawia::Audio {
    class AudioManager;
}

namespace Nawia::Core {
    class EntityManager;
    class Settings;
    class ResourceManager;
}

namespace Nawia::Game {
    class QuestManager;
    class BossManager;
    class SaveGameManager;
    struct BossData;
    struct SaveSlotInfo;
}

namespace Nawia::World {
    class LevelManager;
    struct LevelInfo;
}

namespace Nawia::Item {
    class Backpack;
}

namespace Nawia::UI {

    class StatsUI;
    class SettingsMenu;
    class LevelSelectMenu;

    /**
     * @enum MenuAction
     * @brief Akcja wybrana przez gracza w menu.
     */
    enum class MenuAction {
        None,
        Play,
        NewGame,
        ContinueGame,
        SaveGame,
        LoadGame,
        MainMenu,
        Settings,
        Authors,
        Respawn,
        Exit
    };

    /**
     * @struct MenuButtonDef
     * @brief Opis przycisku menu i akcji, ktora uruchamia.
     */
    struct MenuButtonDef {
        const char* label;
        MenuAction action;
    };

    /**
     * @class UIHandler
     * @brief Koordynuje glowne ekrany UI, HUD i interakcje z ekwipunkiem.
     *
     * UIHandler trzyma shared_ptr do gracza i posiada wlasne komponenty UI.
     * Surowe wskazniki sa nieposiadajacymi referencjami do managerow
     * zyjacych w Engine.
     */
    class UIHandler {
    public:
        UIHandler();
        ~UIHandler();

        /** @brief Podpina UI do aktualnego gracza i dlugo zyjacych managerow Engine. */
        void initialize(const std::shared_ptr<Entity::Player>& player, Core::EntityManager* entity_manager, Core::ResourceManager& resource_manager, Game::QuestManager* quest_manager, const Core::Settings* settings);
        
        /** @brief Aktualizuje przejsciowy stan UI, np. powiadomienia i gladkie paski. */
        void update(float dt);
        /** @brief Renderuje HUD oraz otwarte panele rozgrywki. */
        void render(const Core::GameCamera& camera, const Game::BossManager* boss_manager = nullptr);
        void renderDialogueOnly();
        void renderMainMenu() const;
        void renderSettingsMenu() const;
        void renderSaveSlotMenu() const;
        
        MenuAction handleMenuInput();
        MenuAction handleSettingsInput();
        MenuAction handlePauseMenuInput(bool saves_enabled = true);
        MenuAction handleGameOverInput();
        int handleSaveSlotInput();

        void renderGameOverScreen() const;
        void handleInput();
        void renderPauseMenu(bool saves_enabled = true) const;

        void renderLevelSelectMenu() const;
        void openLevelSelect(const std::vector<World::LevelInfo>& levels);
        void closeLevelSelect();
        std::string handleLevelSelectInput();

        void openSettings(const Core::Settings& settings);
        [[nodiscard]] bool wereSettingsApplied() const;
        [[nodiscard]] const Core::Settings& getAppliedSettings() const;
        void closeSettingsMenu();

        /** @brief Aktualizuje nieposiadane referencje po zmianie systemow przez poziom/zapis. */
        void setLevelManager(World::LevelManager* level_manager) { _level_manager = level_manager; }
        void setSaveGameManager(const Game::SaveGameManager* save_game_manager) { _save_game_manager = save_game_manager; }
        void setPlayer(const std::shared_ptr<Entity::Player>& player);
        void renderLocationInfo() const;
        void openSaveSlotMenu(const std::vector<Game::SaveSlotInfo>& slots, SaveSlotMenu::Mode mode);
        void closeSaveSlotMenu();
        [[nodiscard]] bool isSaveSlotMenuOpen() const { return _save_slot_menu != nullptr; }
        [[nodiscard]] SaveSlotMenu::Mode getSaveSlotMenuMode() const;

        [[nodiscard]] bool isInventoryOpen() const { return _is_inventory_open; }
        void toggleInventory() { _is_inventory_open = !_is_inventory_open; }

        [[nodiscard]] bool isQuestUIOpen() const { return _is_quest_ui_open; }
        void toggleQuestUI() { _is_quest_ui_open = !_is_quest_ui_open; }

        void openContainer(Entity::InteractiveClickable* container);
        void closeContainer();

        /** @brief Otwiera DialogueUI i zwraca koncowy wezel/stan ukonczenia przez on_close. */
        void openDialogue(const Game::DialogueTree& tree, int start_node_id = 0, std::function<void(int, bool)> on_close = nullptr);
        void openDialogueFacing(
            const Game::DialogueTree& tree,
            const std::shared_ptr<Entity::Entity>& speaker,
            int start_node_id = 0,
            std::function<void(int, bool)> on_close = nullptr);
        void setDialogueAudioManager(Audio::AudioManager* audio_manager) { _dialogueUI.setAudioManager(audio_manager); }
        void closeDialogue() { _dialogueUI.close(); }
        [[nodiscard]] bool isDialogueOpen() const { return _dialogueUI.isOpen(); }

        void openAuthors() { _is_authors_open = true; }
        void closeAuthors() { _is_authors_open = false; }
        [[nodiscard]] bool isAuthorsOpen() const { return _is_authors_open; }

        /** @brief Dodaje krotki komunikat renderowany nad UI rozgrywki. */
        void showNotification(const std::string& text, float duration = 2.0f);

        /** @brief Zwraca, czy input gry powinien zostac zatrzymany przez otwarte UI. */
        [[nodiscard]] bool isInputBlocked() const;

        /** @brief Sprawdza, czy kursor znajduje sie nad aktywnym panelem UI. */
        [[nodiscard]] bool isMouseOverUI() const;
        bool closeOpenWindows();

        void drawSharedMenuBackground() const;
        void drawMenuButton(const Rectangle& rect, const char* text, float hover_timer) const;
        const Font& getFont() const { return _font; }

        /**
         * @brief Wyliczony prostokat wycentrowanego przycisku "powrot" u dolu ekranu.
         *
         * `BACK_BUTTON_BOTTOM_OFFSET` mierzymy od dolnej krawedzi przycisku, dzieki
         * czemu we wszystkich menu odstep od dolu ekranu jest taki sam, niezaleznie
         * od wysokosci samego przycisku.
         */
        [[nodiscard]] static Rectangle getCenteredBackButtonRect(float width_factor = 1.0f, float height_factor = 1.0f);
        void triggerLocationBanner();
        void onLevelLoaded();

    private:
        [[nodiscard]] std::vector<MenuButtonDef> buildMainMenuButtons() const;
        [[nodiscard]] static std::vector<Rectangle> getMainMenuLayout(int button_count);
        void renderMainMenuTitle() const;

        void renderPlayerHealthBar() const;
        void renderPlayerStatusEffects() const;
        void renderPlayerAbilityBar() const;
        void renderPlayerExperienceBar() const;
        void renderBossHealthBar(const Game::BossManager* boss_manager) const;
        void renderBossName(const std::string& name, float x, float y, float bar_width, float spacing) const;
        void renderBossPhaseMarkers(const Game::BossData& boss_data, float x, float y, float bar_width, float bar_height) const;
        void renderBossFightInfo(const Game::BossManager* boss_manager, float x, float y, float bar_width, float bar_height, float spacing) const;
        void renderCombatEntityHealthBars(const Core::GameCamera& camera, const Game::BossManager* boss_manager = nullptr) const;
        void renderVerticalMenu(const char* title, const std::vector<MenuButtonDef>& buttons, bool centered = false) const;
        void updateHoverTimers(float dt, const std::vector<Rectangle>& button_rects);
        void renderAuthorsMenu() const;
        void drawBar(float x, float y, float width, float height, float percentage, Color fg_color, Color bg_color) const;
        void drawOrb(float center_x, float center_y, float radius, float target_percent, float ghost_percent, float wave_speed, Color fill_bright, Color fill_dark, Color bg_color, const char* text, const std::shared_ptr<Texture2D>& frame_texture) const;
        bool handleInventoryPanelInput();
        bool handleContainerPanelInput();
        bool pickUpContainerItem(Item::Backpack& container_inventory, int container_slot);
        void closeContainerIfEmpty(Item::Backpack& container_inventory);
        
        void drawMenuButtonsStack(const std::vector<MenuButtonDef>& buttons, const std::vector<Rectangle>& rects) const;
        [[nodiscard]] int getClickedButtonIndex(const std::vector<Rectangle>& rects) const;

        std::shared_ptr<Entity::Player> _player;
        Core::EntityManager* _entity_manager = nullptr;
        World::LevelManager* _level_manager = nullptr;
        const Game::SaveGameManager* _save_game_manager = nullptr;
        Font _font;
        std::shared_ptr<Texture2D> _main_menu_background;
        std::shared_ptr<Texture2D> _menu_btn_idle;
        std::shared_ptr<Texture2D> _menu_btn_hover;
        std::shared_ptr<Texture2D> _ability_bar_frame;
        std::shared_ptr<Texture2D> _empty_ability_icon;
        std::shared_ptr<Texture2D> _food_icon;
        std::shared_ptr<Texture2D> _hp_orb_frame;
        std::shared_ptr<Texture2D> _level_orb_frame;
        std::vector<float> _hover_timers; ///< Timery animacji hover dla przyciskow menu.
        
        std::unique_ptr<SettingsMenu> _settings_menu;
        std::unique_ptr<LevelSelectMenu> _level_select_menu;
        std::unique_ptr<SaveSlotMenu> _save_slot_menu;
        std::unique_ptr<InventoryUI> _inventory_ui;
        bool _is_inventory_open = false;
        std::unique_ptr<QuestUI> _quest_ui;
        bool _is_quest_ui_open = false;
        bool _is_authors_open = false;
        Game::QuestManager* _quest_manager = nullptr;
        std::unique_ptr<ChestUI> _chest_ui;
        Entity::InteractiveClickable* _current_container = nullptr; ///< Otwarta skrzynia/kontener, nieposiadane.
        std::unique_ptr<StatsUI> _stats_ui;
        DialogueUI _dialogueUI;
        int _previous_hp = -1; ///< Ostatnie HP gracza do wykrywania flasha obrazen.
        float _damage_flash_timer = 0.0f;

        struct Notification {
            std::string text;
            float timer;
            float duration;
        };
        std::vector<Notification> _notifications;
        
        const Core::Settings* _settings = nullptr;
        float _visual_hp_percent = 1.0f;  ///< Wygladzona wartosc HP pokazywana przez orb.
        float _visual_exp_percent = 0.0f; ///< Wygladzona wartosc EXP pokazywana przez orb.
        float _location_banner_timer = 0.0f;
        std::string _last_location_name;
        bool _ignore_next_dt = false; ///< Pomija jedna klatke interpolacji UI po zaladowaniu poziomu.
    };

} // namespace Nawia::UI
