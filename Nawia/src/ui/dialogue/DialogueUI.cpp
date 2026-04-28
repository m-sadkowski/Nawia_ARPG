#include "DialogueUI.h"
#include <GlobalScaling.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

namespace Nawia::UI
{

    void DialogueUI::open(const Game::DialogueTree& tree) 
    {
        _current_tree = tree;
        _current_node_id = 0;
        _is_open = true;
    }

    void DialogueUI::close() 
    {
        _is_open = false;
    }

    void DialogueUI::render(const Font& font) 
    {
        if (!_is_open)
            return;

        const auto* node = _current_tree.getNode(_current_node_id);
        if (!node)
        {
            close(); 
            return;
        }

        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        
        const float panel_width = screen_width * 0.6f;
        const float panel_height = Core::GlobalScaling::scaled(DIALOGUE_BOX_HEIGHT);
        const float bottom_margin = Core::GlobalScaling::scaled(DIALOGUE_BOX_MARGIN);
        
        const float panel_x = (screen_width - panel_width) / 2.0f;
        const float panel_y = screen_height - panel_height - bottom_margin;
        
        const Rectangle panel_rect = { panel_x, panel_y, panel_width, panel_height };
        
        // AAA Premium Panel
        DrawRectangleRec(panel_rect, withAlpha(COLOR_PANEL_BG, 0.98f)); // More opaque to hide HUD behind
        DrawRectangleLinesEx(panel_rect, 2.0f, withAlpha(COLOR_ACCENT, 0.8f));
        DrawRectangleGradientV(static_cast<int>(panel_rect.x), static_cast<int>(panel_rect.y), static_cast<int>(panel_rect.width), static_cast<int>(panel_rect.height / 3.0f), withAlpha(WHITE, 0.05f), withAlpha(WHITE, 0.0f));

        const float name_font_size = Core::GlobalScaling::scaled(32.0f); // Balanced
        const float text_font_size = Core::GlobalScaling::scaled(20.0f);
        const float spacing = Core::GlobalScaling::scaled(1.0f);
        const float content_padding_x = Core::GlobalScaling::scaled(30.0f);
        const float content_padding_y = Core::GlobalScaling::scaled(20.0f);

        // Speaker Name
        DrawTextEx(font, node->speaker_name.c_str(), { panel_rect.x + content_padding_x, panel_rect.y + content_padding_y }, name_font_size, spacing, COLOR_ACCENT);
        
        // Main Text - closer to name
        const float text_y = panel_rect.y + content_padding_y + name_font_size + 8.0f;
        DrawTextEx(font, node->text.c_str(), { panel_rect.x + content_padding_x, text_y }, text_font_size, spacing, COLOR_PARCHMENT);

        // Options - aligned bottom-left within the panel
        const float option_start_y = text_y + text_font_size + 15.0f;
        float current_option_y = option_start_y;
        const Vector2 mouse_position = GetMousePosition();

        for (const auto& option : node->options) 
        {
            const Rectangle option_rect = { panel_rect.x + content_padding_x, current_option_y, panel_rect.width - (content_padding_x * 2.0f), text_font_size + 8.0f };
            const bool is_hovered = CheckCollisionPointRec(mouse_position, option_rect);
            
            const std::string option_text = "> " + option.text;
            const Color option_color = is_hovered ? COLOR_ACCENT : withAlpha(COLOR_PARCHMENT, 0.6f);
            
            if (is_hovered)
                DrawRectangleRec(option_rect, withAlpha(WHITE, 0.03f));
                
            DrawTextEx(font, option_text.c_str(), { option_rect.x, option_rect.y }, text_font_size, spacing, option_color);
            current_option_y += text_font_size + Core::GlobalScaling::scaled(8.0f);
        }
    }

    bool DialogueUI::handleInput() 
    {
        if (!_is_open)
            return false;

        const auto* node = _current_tree.getNode(_current_node_id);
        if (!node)
            return false;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            const Vector2 mouse_position = GetMousePosition();
            const float screen_width = static_cast<float>(GetScreenWidth());
            const float screen_height = static_cast<float>(GetScreenHeight());
            
            const float panel_width = screen_width * 0.6f;
            const float panel_height = Core::GlobalScaling::scaled(DIALOGUE_BOX_HEIGHT);
            const float bottom_margin = Core::GlobalScaling::scaled(DIALOGUE_BOX_MARGIN);
            const float text_font_size = Core::GlobalScaling::scaled(20.0f);
            
            const float panel_x = (screen_width - panel_width) / 2.0f;
            const float panel_y = screen_height - panel_height - bottom_margin;
            
            const float name_font_size = Core::GlobalScaling::scaled(32.0f);
            const float content_padding_x = Core::GlobalScaling::scaled(30.0f);
            const float content_padding_y = Core::GlobalScaling::scaled(20.0f);
            
            const float text_y = panel_y + content_padding_y + name_font_size + 8.0f;
            const float option_start_y = text_y + text_font_size + 15.0f;
            
            float current_option_y = option_start_y;

            for (const auto& option : node->options) 
            {
                const Rectangle option_rect = { panel_x + content_padding_x, current_option_y, panel_width - (content_padding_x * 2.0f), text_font_size + 8.0f };

                if (CheckCollisionPointRec(mouse_position, option_rect)) 
                {
                    if (option.action != nullptr)
                        option.action();

                    if (option.next_node_id == -1)
                        close();
                    else
                        _current_node_id = option.next_node_id;

                    return true;
                }
                current_option_y += text_font_size + Core::GlobalScaling::scaled(8.0f);
            }
        }
        
        return true;
    }
} // namespace Nawia::UI