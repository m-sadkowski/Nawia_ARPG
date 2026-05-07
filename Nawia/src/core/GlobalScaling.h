#pragma once

#include <raylib.h>

namespace Nawia::Core {

/**
 * @class GlobalScaling
 * @brief Zarzadza skala interfejsu wzgledem rozdzielczosci ekranu.
 */
class GlobalScaling {
public:
    /// Bazowa rozdzielczosc projektowa.
    static constexpr int BASE_WIDTH = 1920;
    static constexpr int BASE_HEIGHT = 1080;
    static constexpr float BASE_UI_SCALE = 0.9f;

    /**
     * @brief Zwraca aktualny mnoznik skali UI.
     */
    [[nodiscard]] static float getScale() {
        return _scale;
    }

    /**
     * @brief Skaluje wartosc z ukladu projektowego.
     */
    [[nodiscard]] static float scaled(float value) {
        return value * _scale;
    }

    /**
     * @brief Skaluje wartosc calkowita z ukladu projektowego.
     */
    [[nodiscard]] static int scaledInt(int value) {
        return static_cast<int>(static_cast<float>(value) * _scale);
    }

    /**
     * @brief Przelicza skale po zmianie rozdzielczosci lub ustawien UI.
     */
    static void update() {
        const float width_scale = static_cast<float>(GetScreenWidth()) / static_cast<float>(BASE_WIDTH);
        const float height_scale = static_cast<float>(GetScreenHeight()) / static_cast<float>(BASE_HEIGHT);
        const float resolution_scale = (width_scale < height_scale) ? width_scale : height_scale;
        const float requested_scale = resolution_scale * BASE_UI_SCALE * _manual_scale;

        const float max_layout_width = static_cast<float>(GetScreenWidth()) / 1124.0f;
        const float max_layout_height = static_cast<float>(GetScreenHeight()) / 1028.0f;
        const float max_allowed_scale = (max_layout_width < max_layout_height) ? max_layout_width : max_layout_height;

        _scale = (requested_scale < max_allowed_scale) ? requested_scale : max_allowed_scale;
    }

    /**
     * @brief Ustawia reczny mnoznik skali UI.
     */
    static void setManualScale(float scale) {
        _manual_scale = scale;
        update();
    }

    /**
     * @brief Zwraca reczny mnoznik skali UI.
     */
    [[nodiscard]] static float getManualScale() {
        return _manual_scale;
    }

    /**
     * @brief Inicjalizuje system skalowania.
     */
    static void initialize() {
        update();
    }

private:
    static inline float _scale = 1.0f;
    static inline float _manual_scale = 1.0f;  ///< Reczny mnoznik ustawiany przez gracza.
};

} // namespace Nawia::Core
