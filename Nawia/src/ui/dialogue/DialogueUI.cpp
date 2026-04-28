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
        const float scaled_panel_height = Core::GlobalScaling::scaled(DIALOGUE_BOX_HEIGHT);
        const float margin = Core::GlobalScaling::scaled(DIALOGUE_BOX_MARGIN);
        
        const Rectangle panel_rect = { margin, screen_height - scaled_panel_height - margin, screen_width - (margin * 2.0f), scaled_panel_height };
        
        // AAA Premium Panel
        DrawRectangleRec(panel_rect, withAlpha(COLOR_PANEL_BG, 0.95f));
        DrawRectangleLinesEx(panel_rect, 2.0f, withAlpha(COLOR_ACCENT, 0.8f));
        DrawRectangleGradientV(static_cast<int>(panel_rect.x), static_cast<int>(panel_rect.y), static_cast<int>(panel_rect.width), static_cast<int>(panel_rect.height / 4.0f), withAlpha(WHITE, 0.05f), withAlpha(WHITE, 0.0f));

        const float name_font_size = Core::GlobalScaling::scaled(FONT_SIZE_SUBTITLE);
        const float text_font_size = Core::GlobalScaling::scaled(FONT_SIZE_BUTTON);
        const float spacing = Core::GlobalScaling::scaled(1.0f);

        // Speaker Name
        DrawTextEx(font, node->speaker_name.c_str(), { panel_rect.x + 30.0f, panel_rect.y + 20.0f }, name_font_size, spacing, COLOR_ACCENT);
        
        // Main Text
        DrawTextEx(font, node->text.c_str(), { panel_rect.x + 30.0f, panel_rect.y + 20.0f + name_font_size + 10.0f }, text_font_size, spacing, COLOR_PARCHMENT);

        // Options
        float current_option_y = panel_rect.y + scaled_panel_height * 0.6f;
        const Vector2 mouse_pos = GetMousePosition();

        for (const auto& option : node->options) 
        {
            const Rectangle option_rect = { panel_rect.x + 40.0f, current_option_y, panel_rect.width - 80.0f, text_font_size + 10.0f };
            const bool is_hovered = CheckCollisionPointRec(mouse_pos, option_rect);
            
            const std::string option_text = "> " + option.text;
            const Color option_color = is_hovered ? COLOR_ACCENT : withAlpha(COLOR_PARCHMENT, 0.7f);
            
            if (is_hovered)
                DrawRectangleRec(option_rect, withAlpha(WHITE, 0.05f));
                
            DrawTextEx(font, option_text.c_str(), { option_rect.x, option_rect.y }, text_font_size, spacing, option_color);
            current_option_y += text_font_size + Core::GlobalScaling::scaled(10.0f);
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
            const Vector2 mouse_pos = GetMousePosition();
            const float screen_height = static_cast<float>(GetScreenHeight());
            const float scaled_panel_height = Core::GlobalScaling::scaled(DIALOGUE_BOX_HEIGHT);
            const float margin = Core::GlobalScaling::scaled(DIALOGUE_BOX_MARGIN);
            const float text_font_size = Core::GlobalScaling::scaled(FONT_SIZE_BUTTON);
            
            const float panel_x = margin;
            const float panel_width = static_cast<float>(GetScreenWidth()) - (margin * 2.0f);
            float current_option_y = screen_height - scaled_panel_height - margin + scaled_panel_height * 0.6f;

            for (const auto& option : node->options) 
            {
                const Rectangle option_rect = { panel_x + 40.0f, current_option_y, panel_width - 80.0f, text_font_size + 10.0f };

                if (CheckCollisionPointRec(mouse_pos, option_rect)) 
                {
                    if (option.action != nullptr)
                        option.action();

                    if (option.next_node_id == -1)
                        close();
                    else
                        _current_node_id = option.next_node_id;

                    return true;
                }
                current_option_y += text_font_size + Core::GlobalScaling::scaled(10.0f);
            }
        }
        
        return true;
    }
} // namespace Nawia::UI