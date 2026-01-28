#pragma once

#include <string>
#include <vector>
#include <fstream>

namespace Nawia::Core {

/**
 * @struct Resolution
 * @brief Represents a screen resolution with width, height, and display name.
 */
struct Resolution {
    int width;
    int height;
    
    [[nodiscard]] std::string toString() const {
        return std::to_string(width) + " x " + std::to_string(height);
    }
    
    bool operator==(const Resolution& other) const {
        return width == other.width && height == other.height;
    }
};

/**
 * @class Settings
 * @brief Game settings manager with JSON persistence.
 * 
 * Stores user preferences (resolution, fullscreen, etc.) and provides
 * load/save functionality to persist settings across game sessions.
 * 
 * Usage:
 * @code
 *     Settings settings;
 *     settings.load("settings.json");
 *     settings.resolution = {1920, 1080};
 *     settings.save("settings.json");
 * @endcode
 */
class Settings {
public:
    /// Current resolution
    Resolution resolution = {1600, 900};
    
    /// Fullscreen mode enabled
    bool fullscreen = false;
    
    /// UI scale factor (user-controlled)
    float ui_scale = 1.0f;
    
    /// UI scale limits
    static constexpr float UI_SCALE_MIN = 1.0f;
    static constexpr float UI_SCALE_MAX = 1.5f;
    static constexpr float UI_SCALE_STEP = 0.1f;
    
    /// Available resolution presets
    static inline const std::vector<Resolution> AVAILABLE_RESOLUTIONS = {
        {1280, 720},
        {1366, 768},
        {1600, 900},
        {1920, 1080},
        {2560, 1440}
    };
    
    /// Default settings file path
    static constexpr const char* DEFAULT_PATH = "../assets/settings.json";
    
    /**
     * @brief Load settings from JSON file.
     * @param filepath Path to settings file
     * @return true if loaded successfully, false if file doesn't exist or is invalid
     */
    bool load(const std::string& filepath = DEFAULT_PATH);
    
    /**
     * @brief Save settings to JSON file.
     * @param filepath Path to settings file
     * @return true if saved successfully
     */
    [[nodiscard]] bool save(const std::string& filepath = DEFAULT_PATH) const;
    
    /**
     * @brief Get the index of current resolution in AVAILABLE_RESOLUTIONS.
     * @return Index, or 0 if not found
     */
    [[nodiscard]] int getCurrentResolutionIndex() const;
    
    /**
     * @brief Set resolution by index from AVAILABLE_RESOLUTIONS.
     * @param index Index in the available resolutions list
     */
    void setResolutionByIndex(int index);
};

} // namespace Nawia::Core
