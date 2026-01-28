#include "DialogueUI.h"

namespace Nawia::UI {

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

    void DialogueUI::render() 
	{
        if (!_is_open) return;

        const auto* node = _current_tree.getNode(_current_node_id);
        if (!node)
        {
	        close(); 
        	return;
        }

        const float screen_w = static_cast<float>(GetScreenWidth());
        const float screen_h = static_cast<float>(GetScreenHeight());

        const Rectangle panel_rect = { 0, screen_h - PANEL_HEIGHT, screen_w, PANEL_HEIGHT };
        DrawRectangleRec(panel_rect, Fade(BLACK, 0.8f));
        DrawRectangleLinesEx(panel_rect, 2, WHITE);

        DrawText(node->speaker_name.c_str(), 20, static_cast<int>(panel_rect.y) + 10, 20, YELLOW);

        DrawText(node->text.c_str(), 20, static_cast<int>(panel_rect.y) + 40, 20, WHITE);

        float option_y = panel_rect.y + 80;
        for (const auto& option : node->options) 
        {
            DrawText(("- " + option.text).c_str(), 40, static_cast<int>(option_y), 20, LIGHTGRAY);
            option_y += 30;
        }
    }

    bool DialogueUI::handleInput() 
	{
        if (!_is_open) return false;

        const auto* node = _current_tree.getNode(_current_node_id);
        if (!node) return false;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            const Vector2 mouse = GetMousePosition();
            const float screen_h = static_cast<float>(GetScreenHeight());
        	float start_y = screen_h - PANEL_HEIGHT + 80;

            for (const auto& option : node->options) 
            {
                const Rectangle option_rect = { 40, start_y, 500, 25 };

                if (CheckCollisionPointRec(mouse, option_rect)) 
                {
                    if (option.action != nullptr)
                        option.action();

                    if (option.next_node_id == -1)
                        close();
                    else
                        _current_node_id = option.next_node_id;

                    return true;
                }
                start_y += 30;
            }
        }
        return true;
    }
}