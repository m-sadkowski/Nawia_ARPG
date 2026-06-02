#include "FirstLevel.h"

#include <AudioManager.h>
#include <Dialogue.h>
#include <Engine.h>
#include <EntityManager.h>
#include <GlobalScaling.h>
#include <Logger.h>
#include <Map.h>
#include <MathUtils.h>
#include <Player.h>
#include <QuestManager.h>
#include <ResourceManager.h>
#include <SzeptuchaNpc.h>
#include <UIDefines.h>
#include <UIHandler.h>
#include <UIRenderUtils.h>
#include <WandaCorpseNpc.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <json.hpp>
#include <sstream>

namespace Nawia::World {

	namespace {
		constexpr const char* FIRST_LEVEL_MUSIC =
			"assets/audio/music/soulfuljamtracks-slavic-folk-308126.mp3";
		constexpr const char* FIRST_LEVEL_LIGHTING_FILE = "assets/maps/wczora_lighting.json";
		constexpr float WCZORA_INTRO_CAMERA_ZOOM_FACTOR = 0.5f;
		constexpr float WCZORA_INTRO_CAMERA_TARGET_HEIGHT_FACTOR = 0.55f;

		const std::vector<LevelLocationFile> FIRST_LEVEL_LOCATIONS = {
			{"Wczora", "assets/data/locations/wczora.json"},
		};

		void drawIntroParticlesFx(const float width, const float height, const float time) {
			for (int i = 0; i < UI::SMOKE_LAYER_COUNT; ++i) {
				const float seed = static_cast<float>(i) * 11.73f + 3.1f;
				const float travel = UI::fract(UI::hash01(seed) + time * (0.012f + UI::hash01(seed + 2.0f) * 0.016f));
				const float pos_x = width * (0.05f + UI::hash01(seed + 1.0f) * 0.90f) + std::sin(time * (0.22f + UI::hash01(seed + 4.0f) * 0.18f) + seed) * width * 0.06f;
				const float pos_y = height * (1.12f - travel * 1.24f);
				const float radius = Core::GlobalScaling::scaled(110.0f + UI::hash01(seed + 5.0f) * 150.0f);
				const float alpha = (0.35f + (1.0f - travel) * 0.65f) * (0.035f + UI::hash01(seed + 6.0f) * 0.07f);
				DrawCircleGradient(static_cast<int>(pos_x), static_cast<int>(pos_y), radius, UI::withAlpha(LIGHTGRAY, alpha), UI::withAlpha(DARKGRAY, 0.0f));
			}

			for (int i = 0; i < UI::FIRE_PARTICLE_COUNT; ++i) {
				const float seed = static_cast<float>(i) * 17.13f + 8.0f;
				const float cycle = UI::fract(UI::hash01(seed) + time * (0.10f + UI::hash01(seed + 1.0f) * 0.22f));
				const float rise = 1.0f - cycle;
				const float pos_x = width * (0.03f + UI::hash01(seed + 2.0f) * 0.94f) + std::sin(time * (1.0f + UI::hash01(seed + 3.0f) * 1.5f) + seed) * width * (0.01f + UI::hash01(seed + 9.0f) * 0.02f);
				const float pos_y = height * (1.04f - rise * 1.18f);
				const float radius = Core::GlobalScaling::scaled(1.5f + UI::hash01(seed + 7.0f) * UI::hash01(seed + 7.0f) * 12.0f) * (0.45f + rise * 0.95f);
				const float alpha = (0.10f + rise * 0.50f) * (0.55f + UI::hash01(seed + 6.0f) * 0.45f);
				DrawCircleGradient(static_cast<int>(pos_x), static_cast<int>(pos_y), radius, UI::withAlpha(UI::COLOR_GOLDEN_TEXT, alpha), UI::withAlpha(UI::COLOR_SLAVIC_ORANGE, alpha * 0.35f));
			}
		}

		void drawAnimatedIntroImage(const std::shared_ptr<Texture2D>& texture, const float screen_width, const float screen_height, const float time) {
			if (!texture || texture->id <= 0) {
				DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), BLACK);
				return;
			}

			float source_width = static_cast<float>(texture->width);
			float source_height = static_cast<float>(texture->height);
			const float screen_aspect_ratio = screen_width / screen_height;
			const float texture_aspect_ratio = source_width / source_height;

			if (texture_aspect_ratio > screen_aspect_ratio)
				source_width = source_height * screen_aspect_ratio;
			else
				source_height = source_width / screen_aspect_ratio;

			const float zoom_factor = 0.10f + 0.018f * std::sin(time * 0.22f);
			source_width *= (1.0f - zoom_factor);
			source_height *= (1.0f - zoom_factor);

			const float offset_x = std::max(0.0f, (static_cast<float>(texture->width) - source_width) * 0.5f) * std::sin(time * 0.11f + 0.8f);
			const float offset_y = std::max(0.0f, (static_cast<float>(texture->height) - source_height) * 0.5f) * std::cos(time * 0.08f - 0.35f);
			const Rectangle source_rectangle = {
				(static_cast<float>(texture->width) - source_width) * 0.5f + offset_x,
				(static_cast<float>(texture->height) - source_height) * 0.5f + offset_y,
				source_width,
				source_height
			};

			DrawTexturePro(*texture, source_rectangle, {0.0f, 0.0f, screen_width, screen_height}, {0.0f, 0.0f}, 0.0f, WHITE);
		}

		nlohmann::json loadJsonDocument(const std::string& path) {
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

		const nlohmann::json& getIntroConfig() {
			static const nlohmann::json config = loadJsonDocument("assets/data/wczora_intro.json");
			return config;
		}

		bool isPlayerDialogueSpeaker(const std::string& speaker) {
			return speaker == "Logos" || speaker == "Jarko" || speaker == "Player" || speaker == "Gracz";
		}

		bool isPlaceholderOption(const std::string& text) {
			return text.empty() || text == "..." || text == "Dalej";
		}

		std::string resolveFinalOption(const std::string& configured_text, const std::string& current_speaker, const std::string& current_text) {
			if (!isPlaceholderOption(configured_text))
				return configured_text;

			return isPlayerDialogueSpeaker(current_speaker) ? current_text : "Rozumiem.";
		}

		Game::DialogueTree buildLinearDialogueTreeFromJson(
			const nlohmann::json& lines,
			const std::string& final_option_text
		) {
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
					next_line++;
				} else {
					const bool is_final_node = next_line >= lines.size();
					if (is_final_node)
						option.text = resolveFinalOption(final_option_text, node.speaker_name, node.text);
					else
						option.text = isPlayerDialogueSpeaker(node.speaker_name) ? node.text : "Rozumiem.";
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
	}

	std::vector<LevelLocationFile> FirstLevel::getLocationFiles() const {
		return FIRST_LEVEL_LOCATIONS;
	}

	void FirstLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu Wczora...");
		activatePreparedLocations(engine);

		if (engine) {
			engine->getLightingSystem().loadLightingFromJson(FIRST_LEVEL_LIGHTING_FILE);
			engine->getAudioManager().playMusic(FIRST_LEVEL_MUSIC, true, 0.65f);
		}
	}

	void FirstLevel::onExit(Core::Engine* engine) {
		finishIntroSequence(engine);
		Level::onExit(engine);
	}

	void FirstLevel::onNewGameStarted(Core::Engine* engine) {
		startIntroSequence(engine);
	}

	void FirstLevel::handleStoryEvent(Core::Engine* engine, const std::string& event_id, const Vector2 world_position) {
		if (event_id == "wanda_corpse_inspected")
			queueCorpseInspected(world_position);
		(void)engine;
	}

	bool FirstLevel::blocksPlayerControl() const {
		return _intro_phase != IntroPhase::Inactive && _intro_phase != IntroPhase::InspectCorpse;
	}

	bool FirstLevel::isInteractionOnly() const {
		return _intro_phase == IntroPhase::InspectCorpse;
	}

	float FirstLevel::getCameraZoomMultiplier() const {
		return _intro_phase == IntroPhase::Inactive ? 1.0f : WCZORA_INTRO_CAMERA_ZOOM_FACTOR;
	}

	float FirstLevel::getCameraTargetHeightMultiplier() const {
		return _intro_phase == IntroPhase::Inactive ? 1.0f : WCZORA_INTRO_CAMERA_TARGET_HEIGHT_FACTOR;
	}

	void FirstLevel::startIntroSequence(Core::Engine* engine) {
		if (!engine || !engine->getPlayer())
			return;

		removeIntroNpc();
		spawnIntroCorpse(engine);

		_intro_slides.clear();
		const auto& config = getIntroConfig();
		if (config.contains("slides") && config["slides"].is_array()) {
			for (const auto& slide_json : config["slides"]) {
				IntroSlide slide;
				slide.text = slide_json.value("text", "");
				slide.voice_path = slide_json.value("voice_path", "");
				slide.image_path = slide_json.value("image_path", "");
				slide.duration = slide_json.value("duration", 6.0f);
				_intro_slides.push_back(std::move(slide));
			}
		}

		for (auto& slide : _intro_slides) {
			if (!slide.image_path.empty()) {
				slide.image_texture = engine->getResourceManager().getTexture(slide.image_path);
				if (slide.image_texture && slide.image_texture->id > 0)
					SetTextureFilter(*slide.image_texture, TEXTURE_FILTER_TRILINEAR);
			}
		}

		_intro_slide_index = 0;
		_intro_phase = IntroPhase::Slides;
		_intro_timer = 0.0f;
		_intro_overlay_alpha = 1.0f;
		_intro_dialogue_opened = false;
		_intro_flash_timer = 0.0f;
		if (_intro_slides.empty()) {
			openAwakeningDialogue(engine);
			return;
		}
		playSlideVoice(engine);
		engine->getPlayer()->stop();
	}

	void FirstLevel::spawnIntroCorpse(Core::Engine* engine) {
		if (!engine || !engine->getPlayer())
			return;

		for (const auto& entity : engine->getEntityManager().getEntities()) {
			const auto corpse = std::dynamic_pointer_cast<Entity::WandaCorpseNpc>(entity);
			if (corpse) {
				corpse->setDormant(false);
				_intro_corpse = corpse;
				return;
			}
		}

		if (const auto corpse = _intro_corpse.lock())
			corpse->setDormant(true);

		const auto player = engine->getPlayer();
		const float angle = static_cast<float>(GetRandomValue(0, 628)) / 100.0f;
		const float radius = static_cast<float>(GetRandomValue(300, 500)) / 100.0f;
		Vector3 corpse_position = {
			player->getCenter().x + std::cos(angle) * radius,
			player->getAltitude(),
			player->getCenter().y + std::sin(angle) * radius
		};

		if (engine->getCurrentMap() && engine->getCurrentMap()->getNavMesh().isReady())
			corpse_position = engine->getCurrentMap()->getNavMesh().getClosestWalkablePosition(corpse_position);

		auto corpse = std::make_shared<Entity::WandaCorpseNpc>("Zwloki Wandy", corpse_position.x, corpse_position.z, engine);
		corpse->setAltitude(corpse_position.y);
		corpse->setAudioManager(&engine->getAudioManager());
		engine->spawnEntity(corpse);
		_intro_corpse = corpse;
	}

	void FirstLevel::openAwakeningDialogue(Core::Engine* engine) {
		if (!engine)
			return;

		auto player = engine->getPlayer();
		if (player) {
			player->stop();
			const int death_frames = player->getAnimationFrameCount("Death01");
			player->setAnimationSpeed(0.0f);
			player->playAnimation("Death01", false, true, std::max(0, death_frames - 1), true);
		}

		Game::DialogueTree tree = buildSingleNodeDialogueTreeFromJson(getIntroConfig()["awakening_dialogue"]);

		_intro_dialogue_opened = true;
		_intro_phase = IntroPhase::AwakeningDialogue;
		_intro_timer = 0.0f;
		engine->getUIHandler().openDialogue(tree, 0, [this, engine](const int, const bool) {
			if (const auto player = engine->getPlayer()) {
				player->setAnimationSpeed(Entity::Player::DEFAULT_ANIMATION_SPEED);
				player->playAnimation("LayToIdle", false, true, 0, true);
			}
			if (engine->getQuestManager().startQuest("inspect_wanda_body"))
				engine->getUIHandler().showNotification("Nowy quest: Sprawdz zwloki kobiety", 4.0f);
			_intro_phase = IntroPhase::InspectCorpse;
			_intro_timer = 0.0f;
			_intro_dialogue_opened = false;
		});
	}

	void FirstLevel::queueCorpseInspected(const Vector2& corpse_position) {
		_pending_corpse_completion = true;
		queueSzeptuchaEncounter(corpse_position);
	}

	void FirstLevel::queueSzeptuchaEncounter(const Vector2& corpse_position) {
		_pending_szeptucha_encounter = true;
		_pending_szeptucha_position = corpse_position;
		_pending_szeptucha_delay = 0.05f;
	}

	void FirstLevel::startSzeptuchaEncounter(Core::Engine* engine, const Vector2& corpse_position) {
		if (!engine || !engine->getPlayer())
			return;

		if (engine->getUIHandler().isDialogueOpen()) {
			queueSzeptuchaEncounter(corpse_position);
			return;
		}

		removeIntroNpc();

		const auto& szeptucha_config = getIntroConfig()["szeptucha"];
		const auto& spawn_offset = szeptucha_config["spawn_offset"];
		const Vector2 offset = {
			spawn_offset.value("x", 1.8f),
			spawn_offset.value("y", 0.8f)
		};
		Vector2 spawn_2d = {corpse_position.x + offset.x, corpse_position.y + offset.y};
		Vector3 spawn_position = {spawn_2d.x, engine->getPlayer()->getAltitude(), spawn_2d.y};
		if (engine->getCurrentMap() && engine->getCurrentMap()->getNavMesh().isReady())
			spawn_position = engine->getCurrentMap()->getNavMesh().getClosestWalkablePosition(spawn_position);

		auto szeptucha = std::make_shared<Entity::SzeptuchaNpc>("Szeptucha", spawn_position.x, spawn_position.z, engine);
		szeptucha->setAltitude(spawn_position.y);
		szeptucha->setAudioManager(&engine->getAudioManager());
		szeptucha->rotateTowardsCenter(engine->getPlayer()->getCenter().x, engine->getPlayer()->getCenter().y);
		engine->getPlayer()->rotateTowardsCenter(szeptucha->getCenter().x, szeptucha->getCenter().y);
		engine->spawnEntity(szeptucha);
		_intro_npc = szeptucha;
		_intro_flash_timer = 0.55f;
		_intro_phase = IntroPhase::SzeptuchaDialogue;
		_intro_timer = 0.0f;

		Game::DialogueTree tree = buildLinearDialogueTreeFromJson(
			szeptucha_config["lines"],
			szeptucha_config.value("final_option", "Co?"));

		engine->getUIHandler().openDialogue(tree, 0, [this](const int, const bool) {
			removeIntroNpc();
			_intro_flash_timer = 0.55f;
			_pending_final_dialogue = true;
		});
	}

	void FirstLevel::openFinalDialogue(Core::Engine* engine) {
		if (!engine)
			return;

		Game::DialogueTree tree = buildSingleNodeDialogueTreeFromJson(getIntroConfig()["final_dialogue"]);

		_intro_dialogue_opened = true;
		_intro_phase = IntroPhase::FinalDialogue;
		engine->getUIHandler().openDialogue(tree, 0, [this, engine](const int, const bool) {
			finishIntroSequence(engine);
		});
	}

	void FirstLevel::finishIntroSequence(Core::Engine* engine) {
		stopSlideVoice(engine);
		_intro_phase = IntroPhase::Inactive;
		_intro_timer = 0.0f;
		_intro_overlay_alpha = 0.0f;
		_intro_dialogue_opened = false;
		_intro_flash_timer = 0.0f;
		_pending_szeptucha_encounter = false;
		_pending_final_dialogue = false;
		_pending_corpse_completion = false;
		_pending_szeptucha_delay = 0.0f;
	}

	void FirstLevel::removeIntroNpc() {
		if (const auto npc = _intro_npc.lock())
			npc->setDormant(true);
		_intro_npc.reset();
	}

	void FirstLevel::playSlideVoice(Core::Engine* engine) {
		stopSlideVoice(engine);
		if (!engine || _intro_slide_index >= _intro_slides.size())
			return;

		const auto& slide = _intro_slides[_intro_slide_index];
		if (slide.voice_path.empty())
			return;

		_intro_slide_voice_id = "intro_slide:" + slide.voice_path;
		engine->getAudioManager().playSoundFile(_intro_slide_voice_id, slide.voice_path, {1.0f, 1.0f, true});
	}

	void FirstLevel::stopSlideVoice(Core::Engine* engine) {
		if (engine && !_intro_slide_voice_id.empty())
			engine->getAudioManager().stopSound(_intro_slide_voice_id);
		_intro_slide_voice_id.clear();
	}

	void FirstLevel::update(Core::Engine* engine, const float dt) {
		Level::update(engine, dt);

		if (_intro_phase == IntroPhase::Inactive || !engine)
			return;

		if (_pending_szeptucha_delay > 0.0f)
			_pending_szeptucha_delay = std::max(0.0f, _pending_szeptucha_delay - dt);

		if (!engine->getUIHandler().isDialogueOpen()) {
			if (_pending_szeptucha_encounter) {
				if (_pending_szeptucha_delay > 0.0f)
					return;

				const Vector2 position = _pending_szeptucha_position;
				_pending_szeptucha_encounter = false;
				if (_pending_corpse_completion) {
					_pending_corpse_completion = false;
					engine->getQuestManager().completeQuest("inspect_wanda_body", engine);
				}
				startSzeptuchaEncounter(engine, position);
				return;
			}

			if (_pending_final_dialogue) {
				_pending_final_dialogue = false;
				openFinalDialogue(engine);
				return;
			}
		}

		if (auto player = engine->getPlayer(); player && _intro_phase != IntroPhase::InspectCorpse)
			player->stop();

		if (_intro_flash_timer > 0.0f)
			_intro_flash_timer = std::max(0.0f, _intro_flash_timer - dt);

		_intro_timer += dt;
		switch (_intro_phase) {
			case IntroPhase::Slides:
				_intro_overlay_alpha = 1.0f;
				if (_intro_slide_index < _intro_slides.size() &&
					_intro_timer >= _intro_slides[_intro_slide_index].duration) {
					_intro_slide_index++;
					_intro_timer = 0.0f;
					if (_intro_slide_index < _intro_slides.size()) {
						playSlideVoice(engine);
					} else {
						stopSlideVoice(engine);
						if (const auto player = engine->getPlayer()) {
							player->stop();
							const int death_frames = player->getAnimationFrameCount("Death01");
							player->setAnimationSpeed(0.0f);
							player->playAnimation("Death01", false, true, std::max(0, death_frames - 1), true);
						}
						_intro_phase = IntroPhase::FadeFromBlackAfterSlides;
						_intro_overlay_alpha = 1.0f;
					}
				}
				break;
			case IntroPhase::FadeFromBlackAfterSlides:
				_intro_overlay_alpha = std::max(0.38f, 1.0f - _intro_timer / 2.4f);
				if (_intro_timer >= 2.4f)
					openAwakeningDialogue(engine);
				break;
			case IntroPhase::AwakeningDialogue:
				_intro_overlay_alpha = 0.38f;
				break;
			case IntroPhase::InspectCorpse:
				_intro_overlay_alpha = 0.0f;
				break;
			case IntroPhase::SzeptuchaDialogue:
			case IntroPhase::FinalDialogue:
				_intro_overlay_alpha = 0.22f;
				break;
			case IntroPhase::Inactive:
				break;
		}
	}

	void FirstLevel::renderOverlay(Core::Engine* engine) const {
		if (_intro_phase == IntroPhase::Inactive && _intro_flash_timer <= 0.0f)
			return;

		const int width = GetScreenWidth();
		const int height = GetScreenHeight();
		const float alpha = std::clamp(_intro_overlay_alpha, 0.0f, 1.0f);
		const bool rendering_slide = _intro_phase == IntroPhase::Slides &&
			_intro_slide_index < _intro_slides.size();
		if (!rendering_slide && alpha > 0.0f)
			DrawRectangle(0, 0, width, height, Fade(BLACK, alpha));

		if (rendering_slide && engine) {
			const auto& slide = _intro_slides[_intro_slide_index];
			const float fade_in = std::clamp(_intro_timer / 1.2f, 0.0f, 1.0f);
			const float fade_out = std::clamp((slide.duration - _intro_timer) / 1.2f, 0.0f, 1.0f);
			const float text_alpha = std::min(fade_in, fade_out);
			const float screen_width = static_cast<float>(width);
			const float screen_height = static_cast<float>(height);
			const float image_time = static_cast<float>(GetTime()) + static_cast<float>(_intro_slide_index) * 13.0f;

			drawAnimatedIntroImage(slide.image_texture, screen_width, screen_height, image_time);
			DrawRectangleGradientV(0, 0, width, height, UI::withAlpha({30, 14, 10, 255}, 0.10f), UI::withAlpha({5, 5, 8, 255}, 0.55f));
			drawIntroParticlesFx(screen_width, screen_height, image_time);
			DrawRectangleGradientV(0, 0, width, height, UI::withAlpha(UI::COLOR_ACCENT, 0.02f), UI::withAlpha(BLACK, 0.18f));
			DrawRectangle(0, 0, width, height, Fade(BLACK, 0.22f + (1.0f - text_alpha) * 0.78f));

			const Font& font = engine->getUIHandler().getFont();
			const float font_size = Core::GlobalScaling::scaled(30.0f);
			const float spacing = Core::GlobalScaling::scaled(1.0f);
			const float max_width = screen_width * 0.74f;

			std::vector<std::string> lines;
			std::istringstream paragraphs(slide.text);
			std::string paragraph;
			while (std::getline(paragraphs, paragraph)) {
				if (paragraph.empty()) {
					lines.emplace_back();
					continue;
				}
				std::istringstream words(paragraph);
				std::string word;
				std::string current_line;
				while (words >> word) {
					const std::string candidate = current_line.empty() ? word : current_line + " " + word;
					if (MeasureTextEx(font, candidate.c_str(), font_size, spacing).x <= max_width || current_line.empty()) {
						current_line = candidate;
					} else {
						lines.push_back(current_line);
						current_line = word;
					}
				}
				if (!current_line.empty())
					lines.push_back(current_line);
			}

			const float line_height = font_size * 1.42f;
			const float text_block_height = static_cast<float>(lines.size()) * line_height;
			float text_y = screen_height * 0.66f - text_block_height * 0.5f;
			for (const auto& line : lines) {
				if (line.empty()) {
					text_y += line_height * 0.58f;
					continue;
				}
				const Vector2 size = MeasureTextEx(font, line.c_str(), font_size, spacing);
				const Vector2 pos = {screen_width * 0.5f - size.x * 0.5f, text_y};
				DrawTextEx(font, line.c_str(), {pos.x + 2.0f, pos.y + 2.0f}, font_size, spacing, UI::withAlpha(BLACK, text_alpha * 0.70f));
				DrawTextEx(font, line.c_str(), pos, font_size, spacing, UI::withAlpha(RAYWHITE, text_alpha));
				text_y += line_height;
			}
		}

		if (_intro_flash_timer > 0.0f) {
			const float flash_alpha = std::clamp(_intro_flash_timer / 0.55f, 0.0f, 1.0f);
			DrawRectangle(0, 0, width, height, Fade(WHITE, flash_alpha * 0.72f));
		}
	}

} // namespace Nawia::World
