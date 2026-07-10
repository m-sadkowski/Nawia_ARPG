#pragma once

#include <UIDefines.h>

#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <memory>

namespace Nawia::UI {

    /** @brief Zwraca czesc ulamkowa liczby. */
    inline float fract(float v) { return v - std::floor(v); }

    /** @brief Tworzy deterministyczna wartosc pseudolosowa z zakresu 0..1. */
    inline float hash01(float s) { return fract(std::sin(s * 127.1f) * 43758.5453f); }

    /** @brief Zwraca kolor z podmieniona przezroczystoscia. */
    inline Color withAlpha(Color c, float a) { 
        return { c.r, c.g, c.b, static_cast<unsigned char>(std::clamp(a, 0.0f, 1.0f) * 255.0f) }; 
    }

    /** @brief Interpoluje liniowo dwa kolory. */
    inline Color LerpColor(Color c1, Color c2, float t) {
        return {
            static_cast<unsigned char>(c1.r + (c2.r - c1.r) * t),
            static_cast<unsigned char>(c1.g + (c2.g - c1.g) * t),
            static_cast<unsigned char>(c1.b + (c2.b - c1.b) * t),
            static_cast<unsigned char>(c1.a + (c2.a - c1.a) * t)
        };
    }

    inline void smoothUiTexture(const std::shared_ptr<Texture2D>& texture)
    {
        if (!texture || texture->id <= 0)
            return;

        GenTextureMipmaps(texture.get());
        SetTextureFilter(*texture, TEXTURE_FILTER_TRILINEAR);
    }

    inline void drawPanelFrame(
        const Rectangle rect,
        const float fill_alpha = 0.98f,
        const float border_thickness = 2.0f,
        const Color fill_color = COLOR_PANEL_BG,
        const Color border_color = COLOR_ACCENT,
        const float border_alpha = 0.8f)
    {
        DrawRectangleRec(rect, withAlpha(fill_color, fill_alpha));
        DrawRectangleLinesEx(rect, border_thickness, withAlpha(border_color, border_alpha));
    }

    inline Vector2 centeredTextPosition(
        const Font& font,
        const char* text,
        const Rectangle bounds,
        const float font_size,
        const float spacing)
    {
        const Vector2 text_size = MeasureTextEx(font, text, font_size, spacing);
        return {
            bounds.x + (bounds.width - text_size.x) * 0.5f,
            bounds.y + (bounds.height - text_size.y) * 0.5f
        };
    }

    inline void drawTextWithShadow(
        const Font& font,
        const char* text,
        const Vector2 position,
        const float font_size,
        const float spacing,
        const Color color,
        const Vector2 shadow_offset = {1.0f, 1.0f},
        const Color shadow_color = {0, 0, 0, 153})
    {
        DrawTextEx(font, text, {position.x + shadow_offset.x, position.y + shadow_offset.y}, font_size, spacing, shadow_color);
        DrawTextEx(font, text, position, font_size, spacing, color);
    }

    inline void drawCenteredText(
        const Font& font,
        const char* text,
        const Rectangle bounds,
        const float font_size,
        const float spacing,
        const Color color)
    {
        DrawTextEx(font, text, centeredTextPosition(font, text, bounds, font_size, spacing), font_size, spacing, color);
    }

} // namespace Nawia::UI
