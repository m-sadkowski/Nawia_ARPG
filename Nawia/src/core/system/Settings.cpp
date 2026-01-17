#include "Settings.h"
#include <json.hpp>

namespace Nawia::Core {

bool Settings::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        nlohmann::json j;
        file >> j;
        
        if (j.contains("resolution")) {
            resolution.width = j["resolution"].value("width", 1280);
            resolution.height = j["resolution"].value("height", 720);
        }
        
        fullscreen = j.value("fullscreen", false);
        uiScale = j.value("uiScale", 1.0f);
        
        // Clamp uiScale to valid range
        if (uiScale < UI_SCALE_MIN) uiScale = UI_SCALE_MIN;
        if (uiScale > UI_SCALE_MAX) uiScale = UI_SCALE_MAX;
        
        return true;
    } catch (...) {
        return false;
    }
}

bool Settings::save(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        nlohmann::json j;
        j["resolution"]["width"] = resolution.width;
        j["resolution"]["height"] = resolution.height;
        j["fullscreen"] = fullscreen;
        j["uiScale"] = uiScale;
        
        file << j.dump(4); // Pretty print with 4-space indent
        return true;
    } catch (...) {
        return false;
    }
}

int Settings::getCurrentResolutionIndex() const {
    for (size_t i = 0; i < AVAILABLE_RESOLUTIONS.size(); ++i) {
        if (AVAILABLE_RESOLUTIONS[i] == resolution) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

void Settings::setResolutionByIndex(int index) {
    if (index >= 0 && index < static_cast<int>(AVAILABLE_RESOLUTIONS.size())) {
        resolution = AVAILABLE_RESOLUTIONS[index];
    }
}

} // namespace Nawia::Core
