#pragma once

#include <raylib.h>

namespace Nawia::Core {

/**
 * @class GlobalScaling
 * @brief Singleton managing UI scaling across the game based on screen resolution.
 * 
 * The scaling factor is calculated relative to a base/reference resolution (1920x1080).
 * All UI elements should use scaled() to convert design-time sizes to screen sizes.
 * 
 * Usage example:
 * @code
 *     float buttonWidth = GlobalScaling::scaled(200.0f);  // Returns scaled value
 *     float scale = GlobalScaling::getScale();            // Get raw scale factor
 * @endcode
 */
class GlobalScaling {
public:
    /// Base reference resolution (design-time resolution)
    static constexpr int BASE_WIDTH = 1920;
    static constexpr int BASE_HEIGHT = 1080;

    /**
     * @brief Get the current UI scale factor.
     * @return Scale factor (1.0 = base resolution, 0.5 = half size, 2.0 = double size)
     */
    [[nodiscard]] static float getScale() {
        return _scale;
    }

    /**
     * @brief Scale a value by the current scale factor.
     * @param value The design-time value (at 1920x1080)
     * @return The scaled value for current screen resolution
     */
    [[nodiscard]] static float scaled(float value) {
        return value * _scale;
    }

    /**
     * @brief Scale an integer value by the current scale factor.
     * @param value The design-time value (at 1920x1080)
     * @return The scaled value as integer for current screen resolution
     */
    [[nodiscard]] static int scaledInt(int value) {
        return static_cast<int>(static_cast<float>(value) * _scale);
    }

    /**
     * @brief Recalculate scale factor based on current screen size.
     * Call this after window resize or resolution change.
     * If manual scale is set, it will be multiplied by resolution scale.
     */
    static void update() {
        const float width_scale = static_cast<float>(GetScreenWidth()) / static_cast<float>(BASE_WIDTH);
        const float height_scale = static_cast<float>(GetScreenHeight()) / static_cast<float>(BASE_HEIGHT);
        // Use the smaller scale to ensure UI fits on screen
        const float resolution_scale = (width_scale < height_scale) ? width_scale : height_scale;
        _scale = resolution_scale * _manual_scale;
    }

    /**
     * @brief Set manual UI scale multiplier.
     * @param scale Scale factor (1.0 = normal, 2.0 = double size)
     */
    static void setManualScale(float scale) {
        _manual_scale = scale;
        update();
    }

    /**
     * @brief Get the manual scale factor.
     */
    [[nodiscard]] static float getManualScale() {
        return _manual_scale;
    }

    /**
     * @brief Initialize the scaling system.
     * Call once at engine startup after window creation.
     */
    static void initialize() {
        update();
    }

private:
    static inline float _scale = 1.0f;
    static inline float _manual_scale = 1.0f;  ///< User-controlled scale multiplier
};

} // namespace Nawia::Core
