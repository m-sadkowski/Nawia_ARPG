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
        SetTextureFilter(_font.texture, TEXTURE_FILTER_BILINEAR);
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
        Vector2 mouse_pos = GetMousePosition();
        
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());

        // Button Config - Keep consistent with renderMainMenu
        constexpr float btn_width = 200.0f;
        constexpr float btn_height = 50.0f;
        constexpr float spacing = 20.0f;
        const float start_y = screen_height / 2.0f;
        const float center_x = (screen_width - btn_width) / 2.0f;

        const Rectangle play_btn_rect = { center_x, start_y, btn_width, btn_height };
        const Rectangle exit_btn_rect = { center_x, start_y + btn_height + spacing, btn_width, btn_height };

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
        {
            if (CheckCollisionPointRec(mouse_pos, play_btn_rect))
                return MenuAction::Play;
            if (CheckCollisionPointRec(mouse_pos, exit_btn_rect))
                return MenuAction::Exit;
        }

        return MenuAction::None;
    }

    void UIHandler::render(const Core::Camera& camera) const 
	{
        if (!_player || !_entity_manager) return;

        renderPlayerHealthBar();
        renderEnemyHealthBars(camera);
    }

    void UIHandler::renderMainMenu() const 
	{
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());

        // Draw Background (Semi-transparent overlay if game is running behind, or solid color)
        DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), Fade(BLACK, 0.8f));

        // Draw Title
        const auto* title_text = "NAWIA ARPG";
        constexpr float title_font_size = 60.0f;
        constexpr float spacing = 2.0f;
        Vector2 title_size = MeasureTextEx(_font, title_text, title_font_size, spacing);
        DrawTextEx(_font, title_text, { (screen_width - title_size.x) / 2.0f, screen_height / 4.0f }, title_font_size, spacing, DARKGREEN);

        // Button Config
        constexpr float btn_width = 200.0f;
        constexpr float btn_height = 50.0f;
        constexpr float btn_spacing = 20.0f;
        const float start_y = screen_height / 2.0f;
        const float center_x = (screen_width - btn_width) / 2.0f;

        const Rectangle play_btn_rect = { center_x, start_y, btn_width, btn_height };
        const Rectangle exit_btn_rect = { center_x, start_y + btn_height + btn_spacing, btn_width, btn_height };

        const Vector2 mouse_pos = GetMousePosition();
        const bool mouse_on_play = CheckCollisionPointRec(mouse_pos, play_btn_rect);
        const bool mouse_on_exit = CheckCollisionPointRec(mouse_pos, exit_btn_rect);

        // Draw Play Button
        DrawRectangleRec(play_btn_rect, mouse_on_play ? LIGHTGRAY : RAYWHITE);
        DrawRectangleLinesEx(play_btn_rect, 2, BLACK);
        const auto* play_text = "PLAY";
        Vector2 play_text_size = MeasureTextEx(_font, play_text, 20.0f, 1.0f);
        DrawTextEx(_font, play_text, { center_x + (btn_width - play_text_size.x) / 2.0f, start_y + (btn_height - play_text_size.y) / 2.0f }, 20.0f, 1.0f, BLACK);

        // Draw Exit Button
        DrawRectangleRec(exit_btn_rect, mouse_on_exit ? LIGHTGRAY : RAYWHITE);
        DrawRectangleLinesEx(exit_btn_rect, 2, BLACK);
        const auto* exit_text = "EXIT";
        Vector2 exit_text_size = MeasureTextEx(_font, exit_text, 20.0f, 1.0f);
        DrawTextEx(_font, exit_text, { center_x + (btn_width - exit_text_size.x) / 2.0f, start_y + btn_height + btn_spacing + (btn_height - exit_text_size.y) / 2.0f }, 20.0f, 1.0f, BLACK);
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

    void UIHandler::renderEnemyHealthBars(const Core::Camera& camera) const {
        if (!_entity_manager) return;

        for (const auto& entity : _entity_manager->getEntities())
        {
            if (entity->getFaction() == Entity::Entity::Faction::Enemy && entity->getHP() < entity->getMaxHP() && entity->getHP() > 0) 
            {
                const Vector2 screen_pos = entity->getScreenPos(entity->getX(), entity->getY(), camera.x, camera.y);
                
                // Bar Config
                constexpr float bar_width = 40.0f;
                constexpr float bar_height = 6.0f;
                
                // Calculate dynamic offset based on collider height
                float y_offset = 40.0f; // Default fallback
                if (const auto collider = entity->getCollider()) 
                {
                    if (collider->getType() == Entity::ColliderType::CIRCLE) 
                    {
                        const auto circle = dynamic_cast<Entity::CircleCollider*>(collider);
                        y_offset = circle->getRadius() + 10.0f;
                    } 
                	else if (collider->getType() == Entity::ColliderType::RECTANGLE) 
                    {
                         const auto rect = dynamic_cast<Entity::RectangleCollider*>(collider);
                         y_offset = (rect->getHeight() / 2.0f) + 10.0f;
                    }  
                	else if (collider->getType() == Entity::ColliderType::CONE) 
                    {
                        const auto cone = dynamic_cast<Entity::ConeCollider*>(collider);
                        y_offset = cone->getRadius() + 10.0f;
                    }
                } 
                
                const float bar_x = screen_pos.x - bar_width / 2.0f;
                const float bar_y = screen_pos.y - y_offset;
                
                const float hp_pct = std::clamp(static_cast<float>(entity->getHP()) / static_cast<float>(entity->getMaxHP()), 0.0f, 1.0f);
                
                drawBar(bar_x, bar_y, bar_width, bar_height, hp_pct, RED, DARKGRAY);
                DrawRectangleLinesEx({ bar_x, bar_y, bar_width, bar_height }, 1, BLACK);
            }
        }
    }

    void UIHandler::drawBar(const float x, const float y, const float width, const float height, const float percentage, const Color fg_color, const Color bg_color) const {
        // Background
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height), bg_color);
        
        // Foreground
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width * percentage), static_cast<int>(height), fg_color);
    }

} // namespace Nawia::UI
