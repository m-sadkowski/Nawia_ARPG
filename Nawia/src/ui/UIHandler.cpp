#include "UIHandler.h"
#include "SettingsMenu.h"
#include "LevelSelectMenu.h"
#include "StatsUI.h"

#include <Player.h>
#include <EntityManager.h>
#include <Entity.h>
#include <InteractiveClickable.h>
#include <Constants.h>
#include <GlobalScaling.h>
#include <Settings.h>
#include <Camera.h>
#include <Collider.h>
#include <ResourceManager.h>
#include <QuestManager.h>

#include <algorithm>
#include <iostream>

namespace Nawia::UI {

    namespace {
        struct MenuLayout 
    	{
            Rectangle play_btn;
            Rectangle settings_btn;
            Rectangle exit_btn;
        };

        MenuLayout getMenuLayout(const float screen_width, const float screen_height) 
    	{
            const float btn_width = Core::GlobalScaling::scaled(200.0f);
            const float btn_height = Core::GlobalScaling::scaled(50.0f);
            const float spacing = Core::GlobalScaling::scaled(20.0f);
            
            const float start_y = screen_height / 2.0f;
            const float center_x = (screen_width - btn_width) / 2.0f;

            return {
                { center_x, start_y, btn_width, btn_height },
                { center_x, start_y + btn_height + spacing, btn_width, btn_height },
                { center_x, start_y + 2 * (btn_height + spacing), btn_width, btn_height }
            };
        }

        // Health bar is positioned above the entity in screen space
        // We use a fixed pixel offset above the projected screen position
        constexpr float HEALTH_BAR_Y_OFFSET = 60.0f;
    }

    UIHandler::UIHandler() : _player(nullptr), _entity_manager(nullptr) {}

    UIHandler::~UIHandler() 
	{
        UnloadFont(_font);
    }

    void UIHandler::initialize(const std::shared_ptr<Entity::Player>& player, Core::EntityManager* entity_manager, Core::ResourceManager& resource_manager, Game::QuestManager* quest_manager)
	{
        _player = player;
        _entity_manager = entity_manager;
        // Load font at high resolution for quality scaling
        const int font_size = Core::GlobalScaling::scaledInt(300);
        _font = LoadFontEx("../assets/fonts/slavic_font.ttf", font_size, nullptr, 0);
        GenTextureMipmaps(&_font.texture);
        SetTextureFilter(_font.texture, TEXTURE_FILTER_TRILINEAR);

        _main_menu_bg = resource_manager.getTexture("../assets/textures/main_menu.png");

        _inventory_ui = std::make_unique<InventoryUI>();
        _inventory_ui->loadResources(resource_manager);

        // chest
        _chest_ui = std::make_unique<ChestUI>();
        _current_container = nullptr;

        _stats_ui = std::make_unique<StatsUI>(_player);

        _quest_ui = std::make_unique<QuestUI>();
        _quest_manager = quest_manager;

        _previous_hp = _player->getHP();
    }

    void UIHandler::update(const float dt)
	{
        // Damage Flash Logic
        if (_player)
        {
            int current_hp = _player->getHP();
            if (current_hp < _previous_hp)
            {
                // Player took damage, trigger flash
                std:: cout << "Player took damage" << std::endl;
                _damage_flash_timer = 0.3f; // Flash lasts 0.5 seconds
            }
            _previous_hp = current_hp;
        }

        if (_damage_flash_timer > 0.0f)
        {
            _damage_flash_timer -= dt;
            if (_damage_flash_timer < 0.0f) _damage_flash_timer = 0.0f;
        }

        // Update notifications
        for (auto it = _notifications.begin(); it != _notifications.end();) 
        {
            it->timer -= dt;
            if (it->timer <= 0.0f)
                it = _notifications.erase(it);
            else
                ++it;
        }
    }

    void UIHandler::showNotification(const std::string& text, const float duration) 
	{
        _notifications.push_back({ text, duration, duration });
    }

    void UIHandler::handleInput() 
	{
        if (_dialogueUI.isOpen()) {
            _dialogueUI.handleInput();
            return;
        }

        // Future UI input logic (handled by handleMenuInput for menu state)
        if (IsKeyPressed(KEY_I) || IsKeyPressed(KEY_TAB)) {
            if (_current_container) closeContainer();
            toggleInventory();
        }

        if (IsKeyPressed(KEY_P)) {
            toggleQuestUI();
        }

        if (_is_quest_ui_open) {
            _quest_ui->handleInput();
        }

        if (_is_inventory_open) {
            int _backpack_slot = _inventory_ui->handleInput();
            if (_backpack_slot != -1) {
                _player->equipItemFromBackpack(_backpack_slot);
                return;
            }

            auto _eq_slot = _inventory_ui->getClickedEquipmentSlot();
            if (_eq_slot != Item::EquipmentSlot::None) {
                _player->unequipItem(_eq_slot);
            }

            // chest handler
            if (_current_container) {
                int chestSlot = _chest_ui->handleInput();

                if (chestSlot != -1) {
                    auto& chestInv = *_current_container->getInventory();
                    auto item = chestInv.getItem(chestSlot);

                    if (item) {
                        if (_player->getBackpack().addItem(item)) {
                            chestInv.removeItem(chestSlot);

                            // Notify QuestManager about item collection
                            if (_quest_manager) {
                                _quest_manager->notifyItemCollected(item->getId());
                            }
                        }
                    }
                }
            }
        }
    }

    MenuAction UIHandler::handleMenuInput() 
	{
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const auto layout = getMenuLayout(screen_width, screen_height);
        const Vector2 mouse_pos = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
        {
            if (CheckCollisionPointRec(mouse_pos, layout.play_btn))
                return MenuAction::Play;
            if (CheckCollisionPointRec(mouse_pos, layout.settings_btn))
                return MenuAction::Settings;
            if (CheckCollisionPointRec(mouse_pos, layout.exit_btn))
                return MenuAction::Exit;
        }

        return MenuAction::None;
    }

    void UIHandler::render(const Core::GameCamera& camera) 
	{
        if (!_player || !_entity_manager) return;

        renderPlayerHealthBar();
        renderPlayerAbilityBar();
        renderPlayerExperienceBar();
        renderCombatEntityHealthBars(camera);
        renderLocationInfo();

        // Render damage flash overlay
        if (_damage_flash_timer > 0.0f)
        {
            const float screen_width = static_cast<float>(GetScreenWidth());
            const float screen_height = static_cast<float>(GetScreenHeight());
            
            // Alpha fades out as timer decreases relative to max duration (0.5f)
            // Max alpha around 0.4 (semi-transparent red)
            float alpha = (_damage_flash_timer / 0.5f) * 0.4f;
            
            DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(RED, alpha));
        }

        // Render Notifications

        // Render Notifications
        float notif_y = 10.0f;
        const float screen_width = static_cast<float>(GetScreenWidth());
        for (const auto& notif : _notifications) 
        {
            const float font_size = Core::GlobalScaling::scaled(24.0f);
            const float spacing = Core::GlobalScaling::scaled(1.0f);
            const Vector2 text_size = MeasureTextEx(_font, notif.text.c_str(), font_size, spacing);
            
            const float x = screen_width - text_size.x - 20.0f;
            
            // Background
            DrawRectangle(x - 5, notif_y - 5, text_size.x + 10, text_size.y + 10, Fade(BLACK, 0.7f));
            DrawRectangleLines(x - 5, notif_y - 5, text_size.x + 10, text_size.y + 10, WHITE);
            
            DrawTextEx(_font, notif.text.c_str(), { x, notif_y }, font_size, spacing, WHITE);
            
            notif_y += text_size.y + 20.0f;
        }

        _dialogueUI.render(_font);

        if (_is_inventory_open) {
            _inventory_ui->render(_font, *_player);

             if (_stats_ui) {
                 const float margin = Core::GlobalScaling::scaled(20.0f);
                 // StatsUI height is now scaled(240.0f) inside the class, so we calculate position assuming that
                 const float ui_height = Core::GlobalScaling::scaled(240.0f); 
                 
                 const float stats_x = margin;
                 const float stats_y = static_cast<float>(GetScreenHeight()) - ui_height - margin;
                 
                 _stats_ui->render(stats_x, stats_y, _font);
            }

            if (_current_container) {
                _chest_ui->render(*_current_container->getInventory(), _font);
            }
        }

        if (_is_quest_ui_open) {
            _quest_ui->render(_font, _quest_manager);
        }
    }

    void UIHandler::renderMainMenu() const 
	{
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());

        // Draw Background
        if (_main_menu_bg) {
            DrawTexturePro(*_main_menu_bg, 
                { 0, 0, static_cast<float>(_main_menu_bg->width), static_cast<float>(_main_menu_bg->height) }, 
                { 0, 0, screen_width, screen_height }, 
                { 0, 0 }, 0.0f, WHITE);
        } else {
            DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, 0.8f));
        }

        // Draw Title
        const auto* title_text = "Nawia";
        const float title_font_size = Core::GlobalScaling::scaled(100.0f);
        const float spacing = Core::GlobalScaling::scaled(2.0f);
        Vector2 title_size = MeasureTextEx(_font, title_text, title_font_size, spacing);
        
        Vector2 title_pos = { (screen_width - title_size.x) / 2.0f, screen_height / 3.0f };
        
        // shadow
        const float shadow_offset = Core::GlobalScaling::scaled(4.0f);
        DrawTextEx(_font, title_text, { title_pos.x + shadow_offset, title_pos.y + shadow_offset }, title_font_size, spacing, BLACK);
        // text
        DrawTextEx(_font, title_text, title_pos, title_font_size, spacing, WHITE);

        // Draw Buttons
        const auto layout = getMenuLayout(screen_width, screen_height);
        const Vector2 mouse_pos = GetMousePosition();

        drawMenuButton(layout.play_btn, "GRAJ", CheckCollisionPointRec(mouse_pos, layout.play_btn));
        drawMenuButton(layout.settings_btn, "USTAWIENIA", CheckCollisionPointRec(mouse_pos, layout.settings_btn));
        drawMenuButton(layout.exit_btn, "WYJDZ", CheckCollisionPointRec(mouse_pos, layout.exit_btn));
    }

    void UIHandler::drawMenuButton(const Rectangle& rect, const char* text, const bool is_hovered) const
    {
        DrawRectangleRec(rect, is_hovered ? Fade(WHITE, 0.4f) : Fade(WHITE, 0.15f));
        DrawRectangleLinesEx(rect, Core::GlobalScaling::scaled(2.0f), WHITE);
        
        const float font_size = Core::GlobalScaling::scaled(20.0f);
        const float spacing = Core::GlobalScaling::scaled(1.0f);
        Vector2 text_size = MeasureTextEx(_font, text, font_size, spacing);
        
        Vector2 text_pos = { 
            rect.x + (rect.width - text_size.x) / 2.0f, 
            rect.y + (rect.height - text_size.y) / 2.0f 
        };
        
        DrawTextEx(_font, text, text_pos, font_size, spacing, WHITE);
    }

    void UIHandler::renderPlayerHealthBar() const 
	{
        if (!_player) return;

        // Configuration for Player Health Bar (scaled)
        const float bar_width = Core::GlobalScaling::scaled(300.0f);
        const float bar_height = Core::GlobalScaling::scaled(25.0f);
        const float bottom_margin = Core::GlobalScaling::scaled(50.0f);

        const float screen_x = (static_cast<float>(GetScreenWidth()) - bar_width) / 2.0f;
        const float screen_y = static_cast<float>(GetScreenHeight()) - bottom_margin - bar_height;

        const float hp_pct = std::clamp(static_cast<float>(_player->getHP()) / static_cast<float>(_player->getMaxHP()), 0.0f, 1.0f);

        // Draw Player Bar
        drawBar(screen_x, screen_y, bar_width, bar_height, hp_pct, RED, DARKGRAY);
        
        // Draw Border
        DrawRectangleLinesEx({ screen_x, screen_y, bar_width, bar_height }, Core::GlobalScaling::scaled(2.0f), WHITE);
        
        const float font_size = Core::GlobalScaling::scaled(20.0f);
        const float text_spacing = Core::GlobalScaling::scaled(1.0f);
        const auto* text = TextFormat("%d / %d", _player->getHP(), _player->getMaxHP());
        Vector2 text_size = MeasureTextEx(_font, text, font_size, text_spacing);
        DrawTextEx(_font, text, { screen_x + (bar_width - text_size.x) / 2.0f, screen_y + (bar_height - text_size.y) / 2.0f }, font_size, text_spacing, WHITE);
    }

    void UIHandler::renderCombatEntityHealthBars(const Core::GameCamera& camera) const 
	{
        if (!_entity_manager) return;

        for (const auto& entity : _entity_manager->getEntities())
        {
            if ((entity->getFaction() == Entity::Faction::Enemy || entity->getFaction() == Entity::Faction::Ally) &&
                entity->getHP() < entity->getMaxHP() && 
                entity->getHP() > 0) 
            {
                // Project entity world position to screen
                const Vector2 screen_pos = entity->getScreenPosition(camera.get());
                
                // Bar Config (scaled)
                const float bar_width = Core::GlobalScaling::scaled(40.0f);
                const float bar_height = Core::GlobalScaling::scaled(6.0f);
                
                const float bar_x = screen_pos.x - bar_width / 2.0f;
                const float bar_y = screen_pos.y - HEALTH_BAR_Y_OFFSET * Core::GlobalScaling::getScale();
                
                const float hp_pct = std::clamp(static_cast<float>(entity->getHP()) / static_cast<float>(entity->getMaxHP()), 0.0f, 1.0f);
                
                drawBar(bar_x, bar_y, bar_width, bar_height, hp_pct, RED, DARKGRAY);
                DrawRectangleLinesEx({ bar_x, bar_y, bar_width, bar_height }, Core::GlobalScaling::scaled(1.0f), BLACK);
            }
        }
    }

    void UIHandler::drawBar(const float x, const float y, const float width, const float height, const float percentage, const Color fg_color, const Color bg_color) const 
	{
        // Background
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height), bg_color);
        
        // Foreground
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width * percentage), static_cast<int>(height), fg_color);
    }

	void UIHandler::renderPlayerAbilityBar() const
	{
		if (!_player) return;

		const auto& abilities = _player->getAbilities();
		const float icon_size = Core::GlobalScaling::scaled(50.0f);
		const float spacing = Core::GlobalScaling::scaled(10.0f);
		const float bar_width = (icon_size * 4) + (spacing * 3);
		const float bottom_margin = Core::GlobalScaling::scaled(90.0f); // Positioned above the health bar

		const float start_x = (static_cast<float>(GetScreenWidth()) - bar_width) / 2.0f;
		const float start_y = static_cast<float>(GetScreenHeight()) - bottom_margin - icon_size;
        constexpr int slots = 4;

		for (int i = 0; i < slots; ++i)
		{
			const float x = start_x + (icon_size + spacing) * i;
			const Rectangle rect = { x, start_y, icon_size, icon_size };

			// Draw Background/Empty Slot
			DrawRectangleRec(rect, Fade(BLACK, 0.5f));
			DrawRectangleLinesEx(rect, Core::GlobalScaling::scaled(2.0f), DARKGRAY);

            if (i >= abilities.size())
                continue;
			
			const auto& ability = abilities[i];
			if (const auto icon = ability->getIcon())
				DrawTexturePro(*icon, { 0, 0, static_cast<float>(icon->width), static_cast<float>(icon->height) }, rect, { 0, 0 }, 0.0f, WHITE);

			// Draw Cooldown Overlay
			if (!ability->isReady())
			{
				const float cooldown_ratio = ability->getCooldownTimer() / ability->getStats().cooldown;
				const float overlay_height = icon_size * cooldown_ratio;

				DrawRectangle(static_cast<int>(x), static_cast<int>(start_y), static_cast<int>(icon_size), static_cast<int>(overlay_height), Fade(GRAY, 0.8f));
					
				// Draw Cooldown Text
				const auto* text = TextFormat("%.1f", ability->getCooldownTimer());
				const float font_size = Core::GlobalScaling::scaled(20.0f);
				const float text_spacing = Core::GlobalScaling::scaled(1.0f);
				const Vector2 text_size = MeasureTextEx(_font, text, font_size, text_spacing);
				const Vector2 text_pos = { x + (icon_size - text_size.x) / 2.0f, start_y + (icon_size - text_size.y) / 2.0f };
				DrawTextEx(_font, text, text_pos, font_size, text_spacing, WHITE);
			}
		}
	}

    void UIHandler::renderPlayerExperienceBar() const
    {
        if (!_player) return;

        // Ability bar metrics to align with it
        const float ability_icon_size = Core::GlobalScaling::scaled(50.0f);
        const float ability_spacing = Core::GlobalScaling::scaled(10.0f);
        const float ability_bar_width = (ability_icon_size * 4) + (ability_spacing * 3);
        const float ability_bottom_margin = Core::GlobalScaling::scaled(90.0f);
        
        // Experience bar metrics
        const float bar_height = Core::GlobalScaling::scaled(10.0f);
        const float circle_radius = Core::GlobalScaling::scaled(18.0f);
        const float spacing = Core::GlobalScaling::scaled(8.0f); // Space between circle and bar
        
        const float bar_width = ability_bar_width;
        
        const float start_y = static_cast<float>(GetScreenHeight()) - ability_bottom_margin - ability_icon_size - Core::GlobalScaling::scaled(12.0f) - bar_height;
        const float bar_start_x = (static_cast<float>(GetScreenWidth()) - bar_width) / 2.0f;
        
        // Circle for level
        const float circle_x = bar_start_x - spacing - circle_radius;
        const float circle_y = start_y + bar_height / 2.0f;
        
        int exp_to_next = _player->getExpToNextLvl();
        if (exp_to_next <= 0) exp_to_next = 1;
        const float exp_pct = std::clamp(static_cast<float>(_player->getExp()) / static_cast<float>(exp_to_next), 0.0f, 1.0f);

        // Draw HUD background for xp
        drawBar(bar_start_x, start_y, bar_width, bar_height, exp_pct, PURPLE, DARKGRAY);
        DrawRectangleLinesEx({ bar_start_x, start_y, bar_width, bar_height }, Core::GlobalScaling::scaled(2.0f), WHITE);
        
        // Draw Level Circle
        DrawCircle(static_cast<int>(circle_x), static_cast<int>(circle_y), circle_radius, DARKGRAY);
        DrawCircleLines(static_cast<int>(circle_x), static_cast<int>(circle_y), circle_radius, WHITE);
        
        // Draw Level Text
        const auto* text = TextFormat("%d", _player->getLevel());
        const float font_size = Core::GlobalScaling::scaled(20.0f);
        const float text_spacing = Core::GlobalScaling::scaled(1.0f);
        Vector2 text_size = MeasureTextEx(_font, text, font_size, text_spacing);
        DrawTextEx(_font, text, { circle_x - text_size.x / 2.0f, circle_y - text_size.y / 2.0f }, font_size, text_spacing, WHITE);
    }

    void UIHandler::renderSettingsMenu() const {
        if (_settings_menu) {
            _settings_menu->render(_font);
        }
    }

    MenuAction UIHandler::handleSettingsInput() {
        if (!_settings_menu) {
            return MenuAction::None;
        }
        
        if (_settings_menu->handleInput()) {
            // Back was clicked - close settings and signal to go back
            _settings_menu.reset();
            return MenuAction::Play;  // Signal to return to previous state
        }
        
        return MenuAction::None;
    }

    void UIHandler::openSettings(const Core::Settings& settings) {
        _settings_menu = std::make_unique<SettingsMenu>(settings);
    }

    bool UIHandler::wereSettingsApplied() const {
        return _settings_menu && _settings_menu->wasApplied();
    }

    const Core::Settings& UIHandler::getAppliedSettings() const {
        return _settings_menu->getSettings();
    }

    void UIHandler::closeSettingsMenu() {
        _settings_menu.reset();
    }

    void UIHandler::renderLevelSelectMenu() const {
        if (_level_select_menu) {
            _level_select_menu->render(_font);
        }
    }

    void UIHandler::openLevelSelect(const std::vector<World::LevelInfo>& levels) {
        _level_select_menu = std::make_unique<LevelSelectMenu>(levels);
    }

    void UIHandler::closeLevelSelect() {
        _level_select_menu.reset();
    }

    std::string UIHandler::handleLevelSelectInput() {
        if (_level_select_menu) {
            return _level_select_menu->handleInput();
        }
        return "";
    }

    void UIHandler::renderPauseMenu() const {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        
        // Semi-transparent overlay
        DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, 0.6f));
        
        // Title
        const float title_font_size = Core::GlobalScaling::scaled(40.0f);
        const float spacing = Core::GlobalScaling::scaled(2.0f);
        const char* title = "PAUZA";
        Vector2 title_size = MeasureTextEx(_font, title, title_font_size, spacing);
        DrawTextEx(
            _font, 
            title, 
            {(screen_width - title_size.x) / 2.0f, screen_height / 4.0f}, 
            title_font_size, 
            spacing, 
            WHITE
        );
        
        // Menu buttons (same layout as main menu)
        const auto layout = getMenuLayout(screen_width, screen_height);
        const Vector2 mouse_pos = GetMousePosition();
        
        drawMenuButton(layout.play_btn, "KONTYNUUJ", CheckCollisionPointRec(mouse_pos, layout.play_btn));
        drawMenuButton(layout.settings_btn, "USTAWIENIA", CheckCollisionPointRec(mouse_pos, layout.settings_btn));
        drawMenuButton(layout.exit_btn, "MENU", CheckCollisionPointRec(mouse_pos, layout.exit_btn));
    }

    MenuAction UIHandler::handlePauseMenuInput() {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const auto layout = getMenuLayout(screen_width, screen_height);
        const Vector2 mouse_pos = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mouse_pos, layout.play_btn))
                return MenuAction::Play;  // Resume game
            if (CheckCollisionPointRec(mouse_pos, layout.settings_btn))
                return MenuAction::Settings;
            if (CheckCollisionPointRec(mouse_pos, layout.exit_btn))
                return MenuAction::Exit;  // Quit to main menu
        }

        return MenuAction::None;
    }

    void UIHandler::openContainer(Entity::InteractiveClickable* container) {
        _current_container = container;
        _is_inventory_open = true;
    }

    void UIHandler::closeContainer() {
        _current_container = nullptr;
    }

    bool UIHandler::isInputBlocked() const {
        if (_dialogueUI.isOpen()) return true;
        if (_is_inventory_open) return true;
        if (_current_container) return true;
        if (_is_quest_ui_open) return true;

        return false;
    }

    void UIHandler::renderLocationInfo() const {
        if (!_level_manager) return;

        const std::string level_name = _level_manager->getCurrentLevelName();
        const std::string location_name = _level_manager->getCurrentLocationName();
        if (level_name.empty()) return;

        const float font_size_level = Core::GlobalScaling::scaled(20.0f);
        const float font_size_loc = Core::GlobalScaling::scaled(16.0f);
        const float spacing = Core::GlobalScaling::scaled(1.0f);
        const float padding = Core::GlobalScaling::scaled(10.0f);
        const float margin = Core::GlobalScaling::scaled(40.0f);

        // Measure texts
        const Vector2 level_size = MeasureTextEx(_font, level_name.c_str(), font_size_level, spacing);
        const Vector2 loc_size = MeasureTextEx(_font, location_name.c_str(), font_size_loc, spacing);

        const float box_width = std::max(level_size.x, loc_size.x) + padding * 2.0f;
        const float box_height = level_size.y + loc_size.y + padding * 2.5f;
        const float box_x = margin;
        const float box_y = margin;

        // Background box
        DrawRectangle(static_cast<int>(box_x), static_cast<int>(box_y),
                      static_cast<int>(box_width), static_cast<int>(box_height),
                      Fade(BLACK, 0.6f));
        DrawRectangleLinesEx({box_x, box_y, box_width, box_height},
                             Core::GlobalScaling::scaled(1.0f), Fade(WHITE, 0.3f));

        // Level name
        DrawTextEx(_font, level_name.c_str(),
                   {box_x + padding, box_y + padding},
                   font_size_level, spacing, WHITE);

        // Location name (below level name, slightly indented)
        DrawTextEx(_font, location_name.c_str(),
                   {box_x + padding + Core::GlobalScaling::scaled(5.0f), box_y + padding + level_size.y + Core::GlobalScaling::scaled(4.0f)},
                   font_size_loc, spacing, Fade(WHITE, 0.7f));
    }

    void UIHandler::renderGameOverScreen() const
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());

        // Dark overlay
        DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, 0.75f));

        // Title
        const float title_font_size = Core::GlobalScaling::scaled(65.0f);
        const float spacing = Core::GlobalScaling::scaled(2.0f);
        const char* title = "NIE ZYJESZ";
        Vector2 title_size = MeasureTextEx(_font, title, title_font_size, spacing);
        DrawTextEx(_font, title, {(screen_width - title_size.x) / 2.0f, screen_height / 4.0f}, title_font_size, spacing, RED);

        // Buttons
        const float btn_width = Core::GlobalScaling::scaled(250.0f);
        const float btn_height = Core::GlobalScaling::scaled(50.0f);
        const float btn_spacing = Core::GlobalScaling::scaled(20.0f);
        const float start_y = screen_height / 2.0f;
        const float center_x = (screen_width - btn_width) / 2.0f;

        Rectangle respawn_btn = { center_x, start_y, btn_width, btn_height };
        Rectangle menu_btn = { center_x, start_y + btn_height + btn_spacing, btn_width, btn_height };
        const Vector2 mouse_pos = GetMousePosition();

        drawMenuButton(respawn_btn, "ODRODZENIE", CheckCollisionPointRec(mouse_pos, respawn_btn));
        drawMenuButton(menu_btn, "WROC DO MENU", CheckCollisionPointRec(mouse_pos, menu_btn));
    }

    MenuAction UIHandler::handleGameOverInput()
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());

        const float btn_width = Core::GlobalScaling::scaled(250.0f);
        const float btn_height = Core::GlobalScaling::scaled(50.0f);
        const float btn_spacing = Core::GlobalScaling::scaled(20.0f);
        const float start_y = screen_height / 2.0f;
        const float center_x = (screen_width - btn_width) / 2.0f;

        Rectangle respawn_btn = { center_x, start_y, btn_width, btn_height };
        Rectangle menu_btn = { center_x, start_y + btn_height + btn_spacing, btn_width, btn_height };
        const Vector2 mouse_pos = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            if (CheckCollisionPointRec(mouse_pos, respawn_btn)) {
                return MenuAction::Respawn;
            }
            if (CheckCollisionPointRec(mouse_pos, menu_btn)) {
                return MenuAction::Exit;
            }
        }
        return MenuAction::None;
    }

} // namespace Nawia::UI

