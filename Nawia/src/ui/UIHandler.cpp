#include "UIHandler.h"
#include <Player.h>
#include <EntityManager.h>
#include <Entity.h>
#include <Constants.h>
#include <Camera.h>
#include <Collider.h>
#include <algorithm>
#include <iostream>

namespace Nawia::UI {

    namespace {
        struct MenuLayout 
    	{
            Rectangle play_btn;
            Rectangle exit_btn;
        };

        MenuLayout getMenuLayout(const float screen_width, const float screen_height) 
    	{
            constexpr float btn_width = 200.0f;
            constexpr float btn_height = 50.0f;
            constexpr float spacing = 20.0f;
            
            const float start_y = screen_height / 2.0f;
            const float center_x = (screen_width - btn_width) / 2.0f;

            return {
                { center_x, start_y, btn_width, btn_height },
                { center_x, start_y + btn_height + spacing, btn_width, btn_height }
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

    void UIHandler::initialize(const std::shared_ptr<Entity::Player>& player, Core::EntityManager* entity_manager) 
	{
        _player = player;
        _entity_manager = entity_manager;
        _font = LoadFontEx("../assets/fonts/slavic_font.ttf", 300, nullptr, 0);
        GenTextureMipmaps(&_font.texture);
        SetTextureFilter(_font.texture, TEXTURE_FILTER_TRILINEAR);
    }

    void UIHandler::update(float dt) 
	{
        // Future UI update logic (animations, etc)
    }

    void UIHandler::handleInput() 
	{
        // Future UI input logic (handled by handleMenuInput for menu state)
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
    }

    void UIHandler::renderMainMenu() const 
	{
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());

        // Draw Background
        DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, 0.8f));

        // Draw Title
        const auto* title_text = "NAWIA ARPG";
        constexpr float title_font_size = 60.0f;
        constexpr float spacing = 2.0f;
        Vector2 title_size = MeasureTextEx(_font, title_text, title_font_size, spacing);
        DrawTextEx(_font, title_text, { (screen_width - title_size.x) / 2.0f, screen_height / 4.0f }, title_font_size, spacing, DARKGREEN);

        // Draw Buttons
        const auto layout = getMenuLayout(screen_width, screen_height);
        const Vector2 mouse_pos = GetMousePosition();

        drawMenuButton(layout.play_btn, "PLAY", CheckCollisionPointRec(mouse_pos, layout.play_btn));
        drawMenuButton(layout.exit_btn, "EXIT", CheckCollisionPointRec(mouse_pos, layout.exit_btn));
    }

    void UIHandler::drawMenuButton(const Rectangle& rect, const char* text, const bool is_hovered) const
    {
        DrawRectangleRec(rect, is_hovered ? LIGHTGRAY : RAYWHITE);
        DrawRectangleLinesEx(rect, 2, BLACK);
        
        const float font_size = 20.0f;
        const float spacing = 1.0f;
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

        // Configuration for Player Health Bar
        constexpr float bar_width = 300.0f;
        constexpr float bar_height = 25.0f;
        constexpr float bottom_margin = 50.0f;

        const float screen_x = (static_cast<float>(GetScreenWidth()) - bar_width) / 2.0f;
        const float screen_y = static_cast<float>(GetScreenHeight()) - bottom_margin - bar_height;

        const float hp_pct = std::clamp(static_cast<float>(_player->getHP()) / static_cast<float>(_player->getMaxHP()), 0.0f, 1.0f);

        // Draw Player Bar
        drawBar(screen_x, screen_y, bar_width, bar_height, hp_pct, RED, DARKGRAY);
        
        // Draw Border
        DrawRectangleLinesEx({ screen_x, screen_y, bar_width, bar_height }, 2, WHITE);
        
        const auto* text = TextFormat("%d / %d", _player->getHP(), _player->getMaxHP());
        Vector2 text_size = MeasureTextEx(_font, text, 20.0f, 1.0f);
        DrawTextEx(_font, text, { screen_x + (bar_width - text_size.x) / 2.0f, screen_y + (bar_height - text_size.y) / 2.0f }, 20.0f, 1.0f, WHITE);
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
                
                // Bar Config
                constexpr float bar_width = 40.0f;
                constexpr float bar_height = 6.0f;
                
                const float y_offset = getHealthBarYOffset(*entity);
                const float bar_x = screen_center.x - bar_width / 2.0f;
                const float bar_y = screen_center.y - y_offset;
                
                const float hp_pct = std::clamp(static_cast<float>(entity->getHP()) / static_cast<float>(entity->getMaxHP()), 0.0f, 1.0f);
                
                drawBar(bar_x, bar_y, bar_width, bar_height, hp_pct, RED, DARKGRAY);
                DrawRectangleLinesEx({ bar_x, bar_y, bar_width, bar_height }, 1, BLACK);
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
		constexpr float icon_size = 50.0f;
		constexpr float spacing = 10.0f;
		constexpr float bar_width = (icon_size * 4) + (spacing * 3);
		constexpr float bottom_margin = 90.0f; // Positioned above the health bar (which is at 50 margin + 25 height)

		const float start_x = (static_cast<float>(GetScreenWidth()) - bar_width) / 2.0f;
		const float start_y = static_cast<float>(GetScreenHeight()) - bottom_margin - icon_size;

		for (int i = 0; i < 4; ++i)
		{
			const float x = start_x + (icon_size + spacing) * i;
			const Rectangle rect = { x, start_y, icon_size, icon_size };

			// Draw Background/Empty Slot
			DrawRectangleRec(rect, Fade(BLACK, 0.5f));
			DrawRectangleLinesEx(rect, 2, DARKGRAY);

            if (i >= abilities.size())
                return;
			
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
				constexpr float font_size = 20.0f;
				const Vector2 text_size = MeasureTextEx(_font, text, font_size, 1.0f);
				const Vector2 text_pos = { x + (icon_size - text_size.x) / 2.0f, start_y + (icon_size - text_size.y) / 2.0f };
				DrawTextEx(_font, text, text_pos, font_size, 1.0f, WHITE);
			}
		}
	}

} // namespace Nawia::UI
