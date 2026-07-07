#pragma once

#include <Dialogue.h>
#include <Level.h>

#include <json.hpp>
#include <raylib.h>

#include <string>
#include <vector>

namespace Nawia::World::FirstLevelSupport {

	inline constexpr const char* MUSIC_PATH = "assets/audio/music/soulfuljamtracks-slavic-folk-308126.mp3";
	inline constexpr const char* FOREST_LIGHTING_FILE = "assets/maps/wczora_las_lighting.json";
	inline constexpr const char* NAWIA_LIGHTING_FILE = "assets/maps/wczora_przedsionek_nawii_lighting.json";
	inline constexpr float INTRO_CAMERA_ZOOM_FACTOR = 0.5f;
	inline constexpr float INTRO_CAMERA_TARGET_HEIGHT_FACTOR = 0.55f;
	inline constexpr float SLIDE_VOICE_VOLUME = 0.3f;
	inline constexpr float SLIDE_VOICE_TAIL_SECONDS = 1.25f;
	inline constexpr float SZEPTUCHA_DEFAULT_FORWARD_OFFSET = 4.0f;
	inline constexpr float SZEPTUCHA_DEFAULT_SIDE_OFFSET = 1.2f;
	inline constexpr int PRESENTATION_BOOTS_ITEM_ID = 19;

	[[nodiscard]] const std::vector<LevelLocationFile>& locationFiles();
	[[nodiscard]] const char* getLightingFileForLocation(const LocationDefinition& location);
	[[nodiscard]] bool isPrzedsionekNawiiLocation(const LocationDefinition& location);

	[[nodiscard]] const nlohmann::json& introConfig();
	[[nodiscard]] const nlohmann::json& outroConfig();
	[[nodiscard]] float resolveSlideDuration(const nlohmann::json& slide_json, float fallback_duration);

	[[nodiscard]] Game::DialogueTree buildLinearDialogueTreeFromJson(
		const nlohmann::json& lines,
		const std::string& final_option_text
	);
	[[nodiscard]] Game::DialogueTree buildSingleNodeDialogueTreeFromJson(const nlohmann::json& data);

	[[nodiscard]] Rectangle getIntroSkipButtonRect(int screen_width, int screen_height);
	void drawIntroSkipButton(const Font& font, Rectangle rect);

} // namespace Nawia::World::FirstLevelSupport
