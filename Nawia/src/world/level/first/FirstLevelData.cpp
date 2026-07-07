#include "FirstLevelInternal.h"

#include <GlobalScaling.h>
#include <Logger.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

#include <algorithm>
#include <fstream>

namespace Nawia::World::FirstLevelSupport {

	namespace {

		const std::vector<LevelLocationFile> LOCATIONS = {
			{"", "assets/data/locations/wczora.json"},
			{"", "assets/data/locations/przedsionek_nawii.json"},
		};

		[[nodiscard]] nlohmann::json loadJsonDocument(const std::string& path) {
			std::ifstream file(path);
			if (!file.is_open()) {
				Core::Logger::errorLog("FirstLevel: nie mozna otworzyc pliku JSON: " + path);
				return {};
			}

			nlohmann::json data;
			try {
				file >> data;
			} catch (const nlohmann::json::parse_error&) {
				Core::Logger::errorLog("FirstLevel: blad parsowania JSON: " + path);
				return {};
			}

			return data;
		}

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

		[[nodiscard]] bool isPlayerDialogueSpeaker(const std::string& speaker) {
			return speaker == "Logos" || speaker == "Jarko" || speaker == "Player" || speaker == "Gracz";
		}

		[[nodiscard]] bool isPlaceholderOption(const std::string& text) {
			return text.empty() || text == "..." || text == "Dalej";
		}

		[[nodiscard]] std::string resolveFinalOption(
			const std::string& configured_text,
			const std::string& current_speaker,
			const std::string& current_text)
		{
			if (!isPlaceholderOption(configured_text))
				return configured_text;

			return isPlayerDialogueSpeaker(current_speaker) ? current_text : "Rozumiem.";
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
		static const nlohmann::json config = loadJsonDocument("assets/data/wczora_intro.json");
		return config;
	}

	const nlohmann::json& outroConfig() {
		static const nlohmann::json config = loadJsonDocument("assets/data/wczora_outro.json");
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
		Game::DialogueTree tree;
		if (!lines.is_array())
			return tree;

		for (size_t i = 0; i < lines.size(); ++i) {
			const auto& line = lines[i];
			Game::DialogueNode node;
			node.id = static_cast<int>(i);
			node.speaker_name = line.value("speaker", "");
			node.text = line.value("text", "");
			node.voice_path = line.value("voice_path", "");

			Game::DialogueOption option;
			size_t next_line = i + 1;
			if (next_line < lines.size() && isPlayerDialogueSpeaker(lines[next_line].value("speaker", ""))) {
				option.text = lines[next_line].value("text", "");
			} else {
				const bool is_final_node = next_line >= lines.size();
				option.text = is_final_node
					? resolveFinalOption(final_option_text, node.speaker_name, node.text)
					: "Dalej";
			}
			option.next_node_id = (next_line < lines.size()) ? static_cast<int>(next_line) : -1;
			node.options.push_back(option);
			tree.addNode(node);
		}

		return tree;
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
