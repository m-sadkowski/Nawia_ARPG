#include "FirstLevelInternal.h"

#include <DialogueJson.h>
#include <GlobalScaling.h>
#include <JsonUtils.h>
#include <Logger.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

#include <algorithm>

namespace Nawia::World::FirstLevelSupport {

	namespace {

		const std::vector<LevelLocationFile> LOCATIONS = {
			{"", "assets/data/locations/wczora.json"},
			{"", "assets/data/locations/przedsionek_nawii.json"},
		};

		[[nodiscard]] float getSoundDurationSeconds(const std::string& path) {
			if (path.empty() || !IsAudioDeviceReady())
				return 0.0f;

			const Sound sound = LoadSound(path.c_str());
			if (sound.frameCount == 0 || sound.stream.sampleRate == 0) {
				Core::Logger::errorLog("FirstLevel: nie mozna odczytac dlugosci audio slajdu: " + path);
				return 0.0f;
			}

			const float duration = static_cast<float>(sound.frameCount) / static_cast<float>(sound.stream.sampleRate);
			UnloadSound(sound);
			return duration;
		}

	}

	const std::vector<LevelLocationFile>& locationFiles() {
		return LOCATIONS;
	}

	const char* getLightingFileForLocation(const LocationDefinition& location) {
		const std::string filename = location.source_path.filename().string();
		if (filename == "przedsionek_nawii.json" || location.map.model == "wczora_przedsionek_nawii.glb")
			return NAWIA_LIGHTING_FILE;

		return FOREST_LIGHTING_FILE;
	}

	bool isPrzedsionekNawiiLocation(const LocationDefinition& location) {
		return location.source_path.filename().string() == "przedsionek_nawii.json" ||
			   location.map.model == "wczora_przedsionek_nawii.glb";
	}

	const nlohmann::json& introConfig() {
		static const nlohmann::json config = Core::JsonUtils::loadDocument("assets/data/wczora_intro.json", "FirstLevel");
		return config;
	}

	const nlohmann::json& outroConfig() {
		static const nlohmann::json config = Core::JsonUtils::loadDocument("assets/data/wczora_outro.json", "FirstLevel");
		return config;
	}

	float resolveSlideDuration(const nlohmann::json& slide_json, const float fallback_duration) {
		const float configured_duration = slide_json.value("duration", fallback_duration);
		const std::string voice_path = slide_json.value("voice_path", "");
		const float voice_duration = getSoundDurationSeconds(voice_path);
		if (voice_duration <= 0.0f)
			return configured_duration;

		return std::max(configured_duration, voice_duration + SLIDE_VOICE_TAIL_SECONDS);
	}

	Game::DialogueTree buildLinearDialogueTreeFromJson(
		const nlohmann::json& lines,
		const std::string& final_option_text)
	{
		return Game::DialogueJson::buildLinearTree({{"lines", lines}, {"final_option", final_option_text}});
	}

	Game::DialogueTree buildSingleNodeDialogueTreeFromJson(const nlohmann::json& data) {
		Game::DialogueTree tree;
		if (!data.is_object())
			return tree;

		Game::DialogueNode node;
		node.id = 0;
		node.speaker_name = data.value("speaker", "");
		node.text = data.value("text", "");
		node.voice_path = data.value("voice_path", "");

		Game::DialogueOption option;
		option.text = data.value("option", "Rozumiem.");
		option.next_node_id = -1;
		node.options.push_back(option);
		tree.addNode(node);
		return tree;
	}

	Rectangle getIntroSkipButtonRect(const int screen_width, const int screen_height) {
		const float margin = Core::GlobalScaling::scaled(24.0f);
		const float width = Core::GlobalScaling::scaled(150.0f);
		const float height = Core::GlobalScaling::scaled(42.0f);
		return {
			margin,
			static_cast<float>(screen_height) - margin - height,
			width,
			height
		};
	}

	void drawIntroSkipButton(const Font& font, const Rectangle rect) {
		const Vector2 mouse = GetMousePosition();
		const bool hovered = CheckCollisionPointRec(mouse, rect);
		const Color fill = hovered ? Color{74, 65, 55, 232} : Color{43, 39, 34, 220};
		const Color border = hovered ? UI::COLOR_GOLDEN_TEXT : Color{138, 119, 87, 220};
		DrawRectangleRounded(rect, 0.12f, 8, fill);
		DrawRectangleRoundedLinesEx(rect, 0.12f, 8, Core::GlobalScaling::scaled(1.5f), border);

		const char* text = "Pomin";
		const float font_size = Core::GlobalScaling::scaled(20.0f);
		const float spacing = Core::GlobalScaling::scaled(1.0f);
		const Vector2 text_size = MeasureTextEx(font, text, font_size, spacing);
		DrawTextEx(
			font,
			text,
			{rect.x + rect.width * 0.5f - text_size.x * 0.5f, rect.y + rect.height * 0.5f - text_size.y * 0.5f},
			font_size,
			spacing,
			RAYWHITE);
	}

} // namespace Nawia::World::FirstLevelSupport
