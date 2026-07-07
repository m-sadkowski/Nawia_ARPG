#include "StatsUI.h"
#include <GlobalScaling.h>
#include <Player.h>
#include <ResourceManager.h>
#include <Stats.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

namespace Nawia::UI
{
    StatsUI::StatsUI(const std::shared_ptr<Entity::Player>& player) : _player(player) {}

    void StatsUI::loadResources(Core::ResourceManager& resource_manager)
    {
        _background = resource_manager.getTexture("assets/textures/ui/stats.png");
        smoothUiTexture(_background);
    }

    void StatsUI::render(float x, float y, const Font& font) const
    {
        if (!_player)
            return;

        const float width = Core::GlobalScaling::scaled(300.0f);
        const float height = Core::GlobalScaling::scaled(361.0f);
        const Rectangle panel_rect = { x, y, width, height };
        
        if (_background && _background->id > 0)
        {
            DrawTexturePro(
                *_background,
                { 0.0f, 0.0f, static_cast<float>(_background->width), static_cast<float>(_background->height) },
                panel_rect,
                { 0.0f, 0.0f },
                0.0f,
                WHITE);
        }
        else
        {
            drawPanelFrame(panel_rect, 0.95f, 1.5f);
        }
        
        const auto& stats = _player->getStats();
        
        const float title_font_size = Core::GlobalScaling::scaled(22.0f);
        const float text_font_size = Core::GlobalScaling::scaled(18.0f);
        const float spacing = Core::GlobalScaling::scaled(1.0f);
        
        float current_y = y + height * 0.14f;
        const float x_padding = width * 0.16f;
        const float line_height = Core::GlobalScaling::scaled(32.0f);

        drawCenteredText(font, "STATYSTYKI", { x, current_y, width, title_font_size }, title_font_size, spacing, COLOR_ACCENT);
        current_y += line_height * 1.35f;
        
        auto draw_stat_row = [&](const char* label, const char* value, Color value_color)
        {
            DrawTextEx(font, label, { x + x_padding, current_y }, text_font_size, spacing, withAlpha(COLOR_PARCHMENT, 0.7f));
            
            const Vector2 value_size = MeasureTextEx(font, value, text_font_size, spacing);
            DrawTextEx(font, value, { x + width - x_padding - value_size.x, current_y }, text_font_size, spacing, value_color);
            
            // Delikatny separator wiersza.
            DrawLineEx({ x + x_padding, current_y + text_font_size + 4.0f }, { x + width - x_padding, current_y + text_font_size + 4.0f }, 1.0f, withAlpha(WHITE, 0.05f));
            
            current_y += line_height;
        };
        
        draw_stat_row("ZDROWIE", TextFormat("%d / %d", _player->getHP(), stats.max_hp), RED);
        draw_stat_row("SILA", TextFormat("%d", stats.damage), COLOR_SLAVIC_ORANGE);
        draw_stat_row("PREDKOSC ATAKU", TextFormat("%.2f", stats.attack_speed), COLOR_GOLDEN_TEXT);
        draw_stat_row("PREDKOSC", TextFormat("%.2f", stats.movement_speed), COLOR_SLAVIC_BLUE);
        draw_stat_row("OBRONA", TextFormat("%d", stats.defense), DARKGRAY);
    }
} // namespace Nawia::UI
