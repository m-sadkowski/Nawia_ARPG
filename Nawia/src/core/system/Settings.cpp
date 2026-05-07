#include "Settings.h"

#include <json.hpp>

#include <algorithm>

namespace Nawia::Core {

	bool Settings::load(const std::string& filepath) 
	{
	    std::ifstream file(filepath);
	    if (!file.is_open()) {
	        return false;
	    }
	    
	    try 
	    {
	        nlohmann::json j;
	        file >> j;
	        
	        if (j.contains("resolution"))
	        {
	            resolution.width = j["resolution"].value("width", 1280);
	            resolution.height = j["resolution"].value("height", 720);
	        }
	        
	        fullscreen = j.value("fullscreen", false);
	        ui_scale = j.value("ui_scale", 1.0f);
	        texture_quality = static_cast<TextureQuality>(j.value("texture_quality", 1));
	        show_fps = j.value("show_fps", false);
	        master_volume = j.value("master_volume", 1.0f);
	        music_volume = j.value("music_volume", 0.7f);
	        effects_volume = j.value("effects_volume", 1.0f);
	        
	        if (ui_scale < UI_SCALE_MIN) ui_scale = UI_SCALE_MIN;
	        if (ui_scale > UI_SCALE_MAX) ui_scale = UI_SCALE_MAX;
	        master_volume = std::clamp(master_volume, AUDIO_VOLUME_MIN, AUDIO_VOLUME_MAX);
	        music_volume = std::clamp(music_volume, AUDIO_VOLUME_MIN, AUDIO_VOLUME_MAX);
	        effects_volume = std::clamp(effects_volume, AUDIO_VOLUME_MIN, AUDIO_VOLUME_MAX);
	        
	        return true;
	    } 
		catch (...) 
	    {
	        return false;
	    }
	}

	bool Settings::save(const std::string& filepath) const 
	{
	    std::ofstream file(filepath);
	    if (!file.is_open())
	        return false;
	    
	    try 
		{
	        nlohmann::json j;
	        j["resolution"]["width"] = resolution.width;
	        j["resolution"]["height"] = resolution.height;
	        j["fullscreen"] = fullscreen;
	        j["ui_scale"] = ui_scale;
	        j["texture_quality"] = static_cast<int>(texture_quality);
	        j["show_fps"] = show_fps;
	        j["master_volume"] = master_volume;
	        j["music_volume"] = music_volume;
	        j["effects_volume"] = effects_volume;
	        
	        file << j.dump(4);
	        return true;
	    } 
		catch (...) 
		{
	        return false;
	    }
	}

	int Settings::getCurrentResolutionIndex() const {
	    for (size_t i = 0; i < AVAILABLE_RESOLUTIONS.size(); ++i)
	        if (AVAILABLE_RESOLUTIONS[i] == resolution)
	            return static_cast<int>(i);

	    return 0;
	}

	void Settings::setResolutionByIndex(const int index) 
	{
	    if (index >= 0 && index < static_cast<int>(AVAILABLE_RESOLUTIONS.size()))
	        resolution = AVAILABLE_RESOLUTIONS[index];
	}

} // namespace Nawia::Core
