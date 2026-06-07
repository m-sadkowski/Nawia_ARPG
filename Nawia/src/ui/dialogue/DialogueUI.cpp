#include "DialogueUI.h"

#include <AudioManager.h>
#include <GlobalScaling.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Nawia::UI
{
    namespace
    {
        constexpr float PANEL_WIDTH_RATIO = 0.74f;

        struct DialogueLayout
        {
            Rectangle panel_rect;
            Vector2 name_position;
            Vector2 text_position;
            std::vector<std::string> text_lines;
            std::vector<std::vector<std::string>> option_lines;
            std::vector<Rectangle> option_rectangles;
            float name_font_size = 0.0f;
            float text_font_size = 0.0f;
            float text_spacing = 0.0f;
        };

        std::vector<std::string> wrapText(const Font& font, const std::string& text, const float font_size, const float spacing, const float max_width)
        {
            std::vector<std::string> lines;
            std::istringstream words(text);
            std::string word;
            std::string current_line;

            while (words >> word)
            {
                const std::string candidate = current_line.empty() ? word : current_line + " " + word;
                const Vector2 candidate_size = MeasureTextEx(font, candidate.c_str(), font_size, spacing);

                if (candidate_size.x <= max_width || current_line.empty())
                {
                    current_line = candidate;
                    continue;
                }

                lines.push_back(current_line);
                current_line = word;
            }

            if (!current_line.empty())
                lines.push_back(current_line);

            if (lines.empty())
                lines.push_back("");

            return lines;
        }

        DialogueLayout buildDialogueLayout(const Font& font, const Game::DialogueNode& node)
        {
            const float screen_width = static_cast<float>(GetScreenWidth());
            const float screen_height = static_cast<float>(GetScreenHeight());
            const float panel_width = screen_width * PANEL_WIDTH_RATIO;
            const float bottom_margin = Core::GlobalScaling::scaled(DIALOGUE_BOX_MARGIN);
            const float min_panel_height = Core::GlobalScaling::scaled(DIALOGUE_BOX_HEIGHT);
            const float content_padding_x = Core::GlobalScaling::scaled(34.0f);
            const float content_padding_y = Core::GlobalScaling::scaled(22.0f);
            const float name_font_size = Core::GlobalScaling::scaled(34.0f);
            const float text_font_size = Core::GlobalScaling::scaled(24.0f);
            const float text_spacing = Core::GlobalScaling::scaled(1.0f);
            const float line_height = text_font_size + Core::GlobalScaling::scaled(7.0f);
            const float option_height = text_font_size + Core::GlobalScaling::scaled(16.0f);
            const float option_spacing = Core::GlobalScaling::scaled(10.0f);
            const float text_width = panel_width - content_padding_x * 2.0f;
            const float option_text_width = text_width - Core::GlobalScaling::scaled(28.0f);

            DialogueLayout layout;
            layout.name_font_size = name_font_size;
            layout.text_font_size = text_font_size;
            layout.text_spacing = text_spacing;
            layout.text_lines = wrapText(font, node.text, text_font_size, text_spacing, text_width);

            const float text_height = static_cast<float>(layout.text_lines.size()) * line_height;
            float options_height = 0.0f;
            layout.option_lines.reserve(node.options.size());
            for (size_t i = 0; i < node.options.size(); ++i)
            {
                auto lines = wrapText(font, node.options[i].text, text_font_size, text_spacing, option_text_width);
                const float height = std::max(option_height, static_cast<float>(lines.size()) * line_height + Core::GlobalScaling::scaled(12.0f));
                options_height += height;
                if (i + 1 < node.options.size())
                    options_height += option_spacing;
                layout.option_lines.push_back(std::move(lines));
            }

            const float desired_panel_height =
                content_padding_y +
                name_font_size +
                Core::GlobalScaling::scaled(10.0f) +
                text_height +
                Core::GlobalScaling::scaled(16.0f) +
                options_height +
                content_padding_y;

            const float max_panel_height = std::max(min_panel_height, screen_height - bottom_margin - Core::GlobalScaling::scaled(20.0f));
            const float panel_height = std::min(std::max(min_panel_height, desired_panel_height), max_panel_height);
            const float panel_x = (screen_width - panel_width) / 2.0f;
            const float panel_y = screen_height - panel_height - bottom_margin;

            layout.panel_rect = { panel_x, panel_y, panel_width, panel_height };
            layout.name_position = { panel_x + content_padding_x, panel_y + content_padding_y };
            layout.text_position = { panel_x + content_padding_x, layout.name_position.y + name_font_size + Core::GlobalScaling::scaled(10.0f) };

            float current_option_y = layout.text_position.y + text_height + Core::GlobalScaling::scaled(16.0f);
            for (size_t i = 0; i < node.options.size(); ++i)
            {
                const float dynamic_option_height = std::max(
                    option_height,
                    static_cast<float>(layout.option_lines[i].size()) * line_height + Core::GlobalScaling::scaled(12.0f));
                layout.option_rectangles.push_back({
                    panel_x + content_padding_x,
                    current_option_y,
                    text_width,
                    dynamic_option_height
                });
                current_option_y += dynamic_option_height + option_spacing;
            }

            return layout;
        }

        void drawWrappedText(const Font& font, const std::vector<std::string>& lines, const Vector2 position, const float font_size, const float spacing, const Color color)
        {
            const float line_height = font_size + Core::GlobalScaling::scaled(5.0f);
            for (size_t i = 0; i < lines.size(); ++i)
            {
                DrawTextEx(font, lines[i].c_str(), { position.x, position.y + static_cast<float>(i) * line_height }, font_size, spacing, color);
            }
        }
    }

    void DialogueUI::open(const Game::DialogueTree& tree, const int start_node_id, std::function<void(int, bool)> on_close)
    {
        stopCurrentVoice();
        _current_tree = tree;
        _current_node_id = _current_tree.getNode(start_node_id) ? start_node_id : 0;
        _on_close = std::move(on_close);
        _option_rectangles.clear();
        _is_open = true;
        if (_audio_manager)
            _audio_manager->setMusicDuckingFactor(0.5f);
        playCurrentNodeVoice();
    }

    void DialogueUI::close(const bool completed)
    {
        if (!_is_open)
            return;

        const int closed_node_id = _current_node_id;
        auto on_close = std::move(_on_close);
        stopCurrentVoice();
        _is_open = false;
        if (_audio_manager)
            _audio_manager->setMusicDuckingFactor(1.0f);
        _on_close = nullptr;
        _option_rectangles.clear();

        if (on_close)
            on_close(closed_node_id, completed);
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

        const DialogueLayout layout = buildDialogueLayout(font, *node);
        const Vector2 mouse_position = GetMousePosition();
        _option_rectangles = layout.option_rectangles;

        const Rectangle shadow_rect = {
            layout.panel_rect.x + Core::GlobalScaling::scaled(5.0f),
            layout.panel_rect.y + Core::GlobalScaling::scaled(7.0f),
            layout.panel_rect.width,
            layout.panel_rect.height
        };
        DrawRectangleRounded(shadow_rect, 0.08f, 10, withAlpha(BLACK, 0.45f));
        DrawRectangleRounded(layout.panel_rect, 0.08f, 10, withAlpha(Color{18, 20, 24, 255}, 0.96f));
        DrawRectangleRoundedLinesEx(layout.panel_rect, 0.08f, 10, Core::GlobalScaling::scaled(1.5f), withAlpha(COLOR_ACCENT, 0.72f));
        DrawRectangleGradientV(
            static_cast<int>(layout.panel_rect.x),
            static_cast<int>(layout.panel_rect.y),
            static_cast<int>(layout.panel_rect.width),
            static_cast<int>(std::min(layout.panel_rect.height, Core::GlobalScaling::scaled(96.0f))),
            withAlpha(Color{98, 76, 43, 255}, 0.24f),
            withAlpha(WHITE, 0.0f));
        DrawRectangleRec(
            {layout.panel_rect.x, layout.text_position.y - Core::GlobalScaling::scaled(13.0f), layout.panel_rect.width, Core::GlobalScaling::scaled(1.0f)},
            withAlpha(COLOR_ACCENT, 0.34f));

        DrawTextEx(font, node->speaker_name.c_str(), layout.name_position, layout.name_font_size, layout.text_spacing, COLOR_ACCENT);
        drawWrappedText(font, layout.text_lines, layout.text_position, layout.text_font_size, layout.text_spacing, COLOR_PARCHMENT);

        for (size_t i = 0; i < node->options.size(); ++i)
        {
            const Rectangle option_rect = layout.option_rectangles[i];
            const bool is_hovered = CheckCollisionPointRec(mouse_position, option_rect);
            const Color option_color = is_hovered ? COLOR_ACCENT : withAlpha(COLOR_PARCHMENT, 0.6f);

            DrawRectangleRounded(option_rect, 0.18f, 8, is_hovered ? withAlpha(COLOR_ACCENT, 0.12f) : withAlpha(BLACK, 0.20f));
            DrawRectangleRoundedLinesEx(
                option_rect,
                0.18f,
                8,
                Core::GlobalScaling::scaled(1.0f),
                is_hovered ? withAlpha(COLOR_ACCENT, 0.72f) : withAlpha(COLOR_ACCENT, 0.22f));

            drawWrappedText(
                font,
                layout.option_lines[i],
                { option_rect.x + Core::GlobalScaling::scaled(14.0f), option_rect.y + Core::GlobalScaling::scaled(6.0f) },
                layout.text_font_size,
                layout.text_spacing,
                option_color);
        }
    }

    bool DialogueUI::handleInput()
    {
        if (!_is_open)
            return false;

        const auto* node = _current_tree.getNode(_current_node_id);
        if (!node)
            return false;

        if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            return true;

        const Vector2 mouse_position = GetMousePosition();
        if (_option_rectangles.size() != node->options.size())
            return true;

        for (size_t i = 0; i < node->options.size(); ++i)
        {
            if (!CheckCollisionPointRec(mouse_position, _option_rectangles[i]))
                continue;

            const auto& option = node->options[i];
            if (option.action != nullptr)
                option.action();

            if (option.next_node_id == -1)
                close(true);
            else
            {
                _current_node_id = option.next_node_id;
                playCurrentNodeVoice();
            }

            return true;
        }

        return true;
    }

    void DialogueUI::playCurrentNodeVoice()
    {
        stopCurrentVoice();
        if (!_audio_manager)
            return;

        const auto* node = _current_tree.getNode(_current_node_id);
        if (!node || node->voice_path.empty())
            return;

        _current_voice_id = "dialogue_voice:" + node->voice_path;
        _audio_manager->playSoundFile(_current_voice_id, node->voice_path, {1.0f, 1.0f, true});
    }

    void DialogueUI::stopCurrentVoice()
    {
        if (_audio_manager && !_current_voice_id.empty())
            _audio_manager->stopSound(_current_voice_id);

        _current_voice_id.clear();
    }
} // namespace Nawia::UI
