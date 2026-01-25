#include "UIHandler.h"
#include "SettingsMenu.h"
#include "StatsUI.h"
#include <Player.h>
#include <EntityManager.h>
#include <Entity.h>
#include <Chest.h>
#include <Constants.h>
#include <GlobalScaling.h>
#include <Settings.h>
#include <Camera.h>
#include <Collider.h>
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

        float getHealthBarYOffset(const Entity::Entity& entity) 
    	{
            float y_offset = Core::TILE_HEIGHT; // Default fallback for no collider

            if (const auto collider = entity.getCollider()) 
            {
                // Convert world units to approximate screen pixels where 1 unit ~ TILE_HEIGHT pixels roughly in verticality
                constexpr float world_to_screen_factor = static_cast<float>(Core::TILE_HEIGHT);
                switch (collider->getType()) 
            	{
                    case Entity::ColliderType::CIRCLE:
                        if (const auto circle = dynamic_cast<const Entity::CircleCollider*>(collider)) 
                            y_offset = (circle->getRadius() * world_to_screen_factor);
                        break;
                    case Entity::ColliderType::RECTANGLE:
                        if (const auto rect = dynamic_cast<const Entity::RectangleCollider*>(collider)) 
                            y_offset = (rect->getHeight() / 2.0f * world_to_screen_factor);
                        break;
                    case Entity::ColliderType::CONE:
                        if (const auto cone = dynamic_cast<const Entity::ConeCollider*>(collider))
                            y_offset = (cone->getRadius() * world_to_screen_factor);
                        break;
                    default:
                        break;
                }
            }
            return y_offset + Core::TILE_HEIGHT;
        }
    }

    UIHandler::UIHandler() : _player(nullptr), _entity_manager(nullptr) {}

    UIHandler::~UIHandler() 
	{
        UnloadFont(_font);
    }

    void UIHandler::initialize(const std::shared_ptr<Entity::Player>& player, Core::EntityManager* entity_manager, Core::ResourceManager& resource_manager)
	{
        _player = player;
        _entity_manager = entity_manager;
        // Load font at high resolution for quality scaling
        const int font_size = Core::GlobalScaling::scaledInt(300);
        _font = LoadFontEx("../assets/fonts/slavic_font.ttf", font_size, nullptr, 0);
        GenTextureMipmaps(&_font.texture);
        SetTextureFilter(_font.texture, TEXTURE_FILTER_TRILINEAR);

        _inventory_ui = std::make_unique<InventoryUI>();
        _inventory_ui->loadResources(resource_manager);

        // chest
        _chest_ui = std::make_unique<ChestUI>();
        _current_chest = nullptr;

        _stats_ui = std::make_unique<StatsUI>(_player);
    }

    void UIHandler::update(float dt) 
	{
        // Future UI update logic (animations, etc)
    }

    void UIHandler::handleInput() 
	{
        // Future UI input logic (handled by handleMenuInput for menu state)
        if (IsKeyPressed(KEY_I)) {
            if (_current_chest) closeChest();
            else toggleInventory();
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
            if (_current_chest) {
                int chestSlot = _chest_ui->handleInput();

                if (chestSlot != -1) {
                    auto& chestInv = _current_chest->getInventory();
                    auto item = chestInv.getItem(chestSlot);

                    if (item) {
                        if (_player->getBackpack().addItem(item)) {
                            chestInv.removeItem(chestSlot);
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

    void UIHandler::render(const Core::Camera& camera) const 
	{
        if (!_player || !_entity_manager) return;

        renderPlayerHealthBar();
        renderPlayerAbilityBar();
        renderEnemyHealthBars(camera);

        if (_stats_ui) {
                 
                 const float stats_x = Core::GlobalScaling::scaled(220.0f);
                 const float stats_y = Core::GlobalScaling::scaled(150.0f);
                 _stats_ui->render(stats_x, stats_y);
            }

        // FPS Counter (top left corner)
        DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, LIME);

        if (_is_inventory_open) {
            _inventory_ui->render(_font, *_player);

            

            if (_current_chest) {
                _chest_ui->render(_current_chest->getInventory(), _font);
            }
        }
    }

    void UIHandler::renderMainMenu() const 
	{
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());

        // Draw Background
        DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, 0.8f));

        // Draw Title
        const auto* title_text = "NAWIA ARPG";
        const float title_font_size = Core::GlobalScaling::scaled(60.0f);
        const float spacing = Core::GlobalScaling::scaled(2.0f);
        Vector2 title_size = MeasureTextEx(_font, title_text, title_font_size, spacing);
        DrawTextEx(_font, title_text, { (screen_width - title_size.x) / 2.0f, screen_height / 4.0f }, title_font_size, spacing, DARKGREEN);

        // Draw Buttons
        const auto layout = getMenuLayout(screen_width, screen_height);
        const Vector2 mouse_pos = GetMousePosition();

        drawMenuButton(layout.play_btn, "PLAY", CheckCollisionPointRec(mouse_pos, layout.play_btn));
        drawMenuButton(layout.settings_btn, "SETTINGS", CheckCollisionPointRec(mouse_pos, layout.settings_btn));
        drawMenuButton(layout.exit_btn, "EXIT", CheckCollisionPointRec(mouse_pos, layout.exit_btn));
    }

    void UIHandler::drawMenuButton(const Rectangle& rect, const char* text, const bool is_hovered) const
    {
        DrawRectangleRec(rect, is_hovered ? LIGHTGRAY : RAYWHITE);
        DrawRectangleLinesEx(rect, Core::GlobalScaling::scaled(2.0f), BLACK);
        
        const float font_size = Core::GlobalScaling::scaled(20.0f);
        const float spacing = Core::GlobalScaling::scaled(1.0f);
        Vector2 text_size = MeasureTextEx(_font, text, font_size, spacing);
        
        Vector2 text_pos = { 
            rect.x + (rect.width - text_size.x) / 2.0f, 
            rect.y + (rect.height - text_size.y) / 2.0f 
        };
        
        DrawTextEx(_font, text, text_pos, font_size, spacing, BLACK);
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

    void UIHandler::renderEnemyHealthBars(const Core::Camera& camera) const 
	{
        if (!_entity_manager) return;

        for (const auto& entity : _entity_manager->getEntities())
        {
            if (entity->getFaction() == Entity::Entity::Faction::Enemy && 
                entity->getHP() < entity->getMaxHP() && 
                entity->getHP() > 0) 
            {
                // Use collider center (or entity pos) projected to screen space, then offset upwards
                const Vector2 world_center = entity->getCenter();
                const Vector2 screen_center = entity->getIsoPos(world_center.x, world_center.y, camera.x, camera.y);
                
                // Bar Config (scaled)
                const float bar_width = Core::GlobalScaling::scaled(40.0f);
                const float bar_height = Core::GlobalScaling::scaled(6.0f);
                
                const float y_offset = getHealthBarYOffset(*entity) * Core::GlobalScaling::getScale();
                const float bar_x = screen_center.x - bar_width / 2.0f;
                const float bar_y = screen_center.y - y_offset;
                
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

    void UIHandler::renderPauseMenu() const {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        
        // Semi-transparent overlay
        DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, 0.6f));
        
        // Title
        const float title_font_size = Core::GlobalScaling::scaled(40.0f);
        const float spacing = Core::GlobalScaling::scaled(2.0f);
        const char* title = "PAUSED";
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
        
        drawMenuButton(layout.play_btn, "RESUME", CheckCollisionPointRec(mouse_pos, layout.play_btn));
        drawMenuButton(layout.settings_btn, "SETTINGS", CheckCollisionPointRec(mouse_pos, layout.settings_btn));
        drawMenuButton(layout.exit_btn, "QUIT TO MENU", CheckCollisionPointRec(mouse_pos, layout.exit_btn));
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

    void UIHandler::openChest(std::shared_ptr<Entity::Chest> chest) {
        _current_chest = chest;
        _is_inventory_open = true;
    }

    void UIHandler::closeChest() {
        _current_chest = nullptr;
    }

} // namespace Nawia::UI

