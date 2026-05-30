#include "Engine.h"

#include <AssetPreloader.h>
#include <Dialogue.h>
#include <GlobalScaling.h>
#include <LoadingScreen.h>
#include <Logger.h>
#include <MathUtils.h>
#include <PlayerController.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

#include <DemoLevel.h>
#include <DevLevel.h>
#include <Entity.h>
#include <FirstLevel.h>
#include <Level.h>
#include <LevelManager.h>
#include <Map.h>
#include <PlayerAbilityFactory.h>
#include <SoundIds.h>
#include <SzeptuchaNpc.h>
#include <WandaCorpseNpc.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <json.hpp>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Nawia::Core {

	namespace {

		constexpr Vector2 k_initial_player_spawn = {0.0f, 0.0f};
		constexpr const char* MENU_MUSIC_PATH = "assets/audio/music/soulfuljamtracks-slavic-folk-308126.mp3";
		constexpr float k_hover_update_interval = 0.05f;
		constexpr float k_hover_mouse_move_threshold_sq = 1.0f;
		constexpr float k_wczora_intro_camera_zoom_factor = 0.5f;
		constexpr float k_camera_zoom_return_speed = 1.7f;

		void drawWczoraIntroParticlesFx(const float width, const float height, const float time) {
			for (int i = 0; i < UI::SMOKE_LAYER_COUNT; ++i) {
				const float seed = static_cast<float>(i) * 11.73f + 3.1f;
				const float travel = UI::fract(UI::hash01(seed) + time * (0.012f + UI::hash01(seed + 2.0f) * 0.016f));
				const float pos_x = width * (0.05f + UI::hash01(seed + 1.0f) * 0.90f) + std::sin(time * (0.22f + UI::hash01(seed + 4.0f) * 0.18f) + seed) * width * 0.06f;
				const float pos_y = height * (1.12f - travel * 1.24f);
				const float radius = GlobalScaling::scaled(110.0f + UI::hash01(seed + 5.0f) * 150.0f);
				const float alpha = (0.35f + (1.0f - travel) * 0.65f) * (0.035f + UI::hash01(seed + 6.0f) * 0.07f);
				DrawCircleGradient(static_cast<int>(pos_x), static_cast<int>(pos_y), radius, UI::withAlpha(LIGHTGRAY, alpha), UI::withAlpha(DARKGRAY, 0.0f));
			}

			for (int i = 0; i < UI::FIRE_PARTICLE_COUNT; ++i) {
				const float seed = static_cast<float>(i) * 17.13f + 8.0f;
				const float cycle = UI::fract(UI::hash01(seed) + time * (0.10f + UI::hash01(seed + 1.0f) * 0.22f));
				const float rise = 1.0f - cycle;
				const float pos_x = width * (0.03f + UI::hash01(seed + 2.0f) * 0.94f) + std::sin(time * (1.0f + UI::hash01(seed + 3.0f) * 1.5f) + seed) * width * (0.01f + UI::hash01(seed + 9.0f) * 0.02f);
				const float pos_y = height * (1.04f - rise * 1.18f);
				const float radius = GlobalScaling::scaled(1.5f + UI::hash01(seed + 7.0f) * UI::hash01(seed + 7.0f) * 12.0f) * (0.45f + rise * 0.95f);
				const float alpha = (0.10f + rise * 0.50f) * (0.55f + UI::hash01(seed + 6.0f) * 0.45f);
				DrawCircleGradient(static_cast<int>(pos_x), static_cast<int>(pos_y), radius, UI::withAlpha(UI::COLOR_GOLDEN_TEXT, alpha), UI::withAlpha(UI::COLOR_SLAVIC_ORANGE, alpha * 0.35f));
			}
		}

		void drawAnimatedWczoraIntroImage(const std::shared_ptr<Texture2D>& texture, const float screen_width, const float screen_height, const float time) {
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
				Logger::errorLog("Engine: nie mozna otworzyc pliku JSON: " + path);
				return {};
			}

			nlohmann::json data;
			try {
				file >> data;
			} catch (const nlohmann::json::parse_error&) {
				Logger::errorLog("Engine: blad parsowania JSON: " + path);
				return {};
			}

			return data;
		}

		const nlohmann::json& getWczoraIntroConfig() {
			static const nlohmann::json config = loadJsonDocument("assets/data/wczora_intro.json");
			return config;
		}

		Game::DialogueTree buildLinearDialogueTreeFromJson(
			const nlohmann::json& lines,
			const std::string& final_option_text,
			const std::string& continue_option_text = "..."
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
				option.text = (i + 1 < lines.size()) ? continue_option_text : final_option_text;
				option.next_node_id = (i + 1 < lines.size()) ? static_cast<int>(i + 1) : -1;
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
			option.text = data.value("option", "...");
			option.next_node_id = -1;
			node.options.push_back(option);
			tree.addNode(node);
			return tree;
		}

	}

	Engine::Engine() {
		SetTraceLogLevel(LOG_ERROR);
		InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Nawia");
		SetExitKey(0);
		SetTargetFPS(0);
		_audio_manager.initialize();

		_lighting_system.initialize();
		_lighting_system.addLight(System::Renderer::LightingSystem::LIGHT_DIRECTIONAL, {-50.0f, 50.0f, -50.0f}, {0.0f, 0.0f, 0.0f}, WHITE);
		_lighting_system.addLight(System::Renderer::LightingSystem::LIGHT_POINT, {0.0f, 5.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, ORANGE);

		if (_settings.load())
			SetWindowSize(_settings.resolution.width, _settings.resolution.height);

		_audio_manager.setMasterVolume(_settings.master_volume);
		_audio_manager.setMusicVolume(_settings.music_volume);
		_audio_manager.setEffectsVolume(_settings.effects_volume);
		loadGameplaySounds();

		GlobalScaling::setManualScale(_settings.ui_scale);

		_entity_manager = std::make_unique<EntityManager>(this);
		_level_manager = std::make_unique<World::LevelManager>();
		_level_manager->registerLevel(std::make_shared<World::DemoLevel>());
		_level_manager->registerLevel(std::make_shared<World::FirstLevel>());
		_level_manager->registerLevel(std::make_shared<World::DevLevel>());

		_loading_kind = LoadingKind::Startup;
		_loading_manifest = AssetLoadManifest::buildStartupManifest();
		_loading_asset_index = 0;
		_loading_progress = 0.0f;
		_loading_status = "Przygotowywanie...";
		_loading_title = "Ladowanie gry";
		_game_state = GameState::Loading;

		_is_running = true;
	}

	UI::UIHandler& Engine::getUIHandler() const {
		return *_ui_handler;
	}

	Engine::~Engine() {
		if (_level_manager && _level_manager->getCurrentLevel())
			_level_manager->getCurrentLevel()->onExit(this);

		_ui_handler.reset();
		_controller.reset();
		_level_manager.reset();
		_entity_manager.reset();
		_player.reset();
		_boss_manager.clearPreloadedBosses();
		_loottable.clear();
		_item_database.clear();
		Map::clearPreloadedMapModels();
		_resource_manager.clear();
		UI::LoadingScreen::unload();

		CloseWindow();
	}

	bool Engine::isRunning() const {
		return _is_running && !WindowShouldClose();
	}

	std::shared_ptr<Entity::Entity> Engine::getEntityAt(const float screen_x, const float screen_y) const {
		return _entity_manager->getEntityAt(screen_x, screen_y, _camera.get());
	}

	void Engine::spawnEntity(std::shared_ptr<Entity::Entity> new_entity) const {
		if (new_entity)
			_entity_manager->addEntity(std::move(new_entity));
	}

	Map* Engine::getCurrentMap() const {
		if (_level_manager && _level_manager->getCurrentLevel())
			return _level_manager->getCurrentLevel()->getMap();

		return nullptr;
	}

	void Engine::processLoading() {
		if (_loading_kind == LoadingKind::None)
			return;

		const size_t total_assets = _loading_manifest.size();
		if (_loading_asset_index < total_assets) {
			const auto& entry = _loading_manifest.entries()[_loading_asset_index];
			AssetPreloader::loadManifestStep(_loading_manifest, _loading_asset_index, _resource_manager);
			_loading_asset_index++;
			_loading_progress = static_cast<float>(_loading_asset_index) / static_cast<float>(total_assets);
			_loading_status = entry.label;
			return;
		}

		if (_loading_kind == LoadingKind::Startup) {
			finishStartupLoading();
			return;
		}

		if (_loading_kind == LoadingKind::Level) {
			finishLevelLoading();
		}
	}

	void Engine::finishStartupLoading() {
		_loading_status = "Inicjalizacja systemow...";
		_loading_progress = 1.0f;

		_item_database.loadDatabase("assets/data/items.json", _resource_manager);
		Logger::debugLog("Zaladowano baze danych przedmiotow");

		_loottable.loadLootTables("assets/data/loottables.json", _item_database);
		_quest_manager.loadFromJson("assets/data/quests.json");
		_boss_manager.loadFromJson("assets/data/bosses.json");

		Entity::Entity::setSharedResourceManager(&_resource_manager);
		createFreshPlayer(true);

		_ui_handler = std::make_unique<UI::UIHandler>();
		_ui_handler->initialize(_player, _entity_manager.get(), _resource_manager, &_quest_manager, &_settings);
		_ui_handler->setDialogueAudioManager(&_audio_manager);
		_ui_handler->setLevelManager(_level_manager.get());
		_ui_handler->setSaveGameManager(&_save_game_manager);

		_audio_manager.playMusic(MENU_MUSIC_PATH, true, 1.f);

		_loading_kind = LoadingKind::None;
		_game_state = GameState::Menu;
	}

	void Engine::queueLevelLoad(
		const std::string& level_name,
		const std::string& initial_location,
		const bool is_new_game,
		const int default_slot
	) {
		const std::string resolved_level = level_name.empty() ? "Demo" : level_name;
		_pending_level_name = resolved_level;
		_pending_initial_location = initial_location;
		_pending_is_new_game = is_new_game;
		_pending_new_game_slot = default_slot;

		_loading_kind = LoadingKind::Level;
		_loading_asset_index = 0;
		_loading_progress = 0.0f;
		_loading_title = "Ladowanie poziomu";
		_loading_status = "Przygotowywanie listy zasobow...";
		_game_state = GameState::Loading;

		const auto level = _level_manager->getRegisteredLevel(resolved_level);
		std::vector<World::LocationDefinition> definitions;
		if (level && !level->getLocationFiles().empty()) {
			const std::string start_location = initial_location.empty()
				? level->getDefaultInitialLocation()
				: initial_location;
			_loading_manifest = AssetLoadManifest::buildForLocationFiles(level->getLocationFiles(), definitions);
			level->setPreparedLocationDefinitions(std::move(definitions), start_location);
		} else {
			_loading_manifest = {};
		}
	}

	void Engine::finishLevelLoading() {
		_loading_status = "Budowanie swiata...";
		_loading_progress = 1.0f;

		Entity::Entity::setSharedResourceManager(&_resource_manager);
		_audio_manager.stopMusic();
		_level_manager->changeLevel(_pending_level_name, this);

		if (_has_pending_save) {
			_save_game_manager.applySaveState(*this, _pending_save_state, _pending_save_slot);
			_has_pending_save = false;
			_pending_save_state = {};
			_game_state = GameState::Playing;
			if (_ui_handler) {
				_ui_handler->onLevelLoaded();
				_ui_handler->showNotification("Gra wczytana.", 3.0f);
			}
		} else {
			_game_state = GameState::Playing;
			if (_ui_handler)
				_ui_handler->onLevelLoaded();

			if (_pending_is_new_game && _pending_new_game_slot > 0)
				saveCurrentGame(_pending_new_game_slot);

			if (_pending_is_new_game && _pending_level_name == "Wczora")
				startWczoraIntroSequence();
		}

		_loading_kind = LoadingKind::None;
		_pending_is_new_game = false;
		_pending_new_game_slot = 0;
	}

	void Engine::createFreshPlayer(const bool grant_starter_items) {
		_player = Entity::PlayerBuilder(this).setPosition(k_initial_player_spawn).build();
		_player->setAudioManager(&_audio_manager);

		const auto& player_setup = Entity::PlayerAbilityFactory::getPlayerSetupConfig();
		for (const auto& ability : Entity::PlayerAbilityFactory::createUnarmedAbilities(player_setup, _resource_manager))
			_player->addAbility(ability);

		if (grant_starter_items) {
			if (const auto fireball = Entity::PlayerAbilityFactory::createStarterFireball(player_setup, _resource_manager))
				_player->addAbility(fireball);
		}

		_controller = std::make_unique<PlayerController>(this, _player);

		if (_entity_manager) {
			_entity_manager->setPlayer(_player);
			_entity_manager->clearNonPlayerEntities();
		}

		if (_ui_handler)
			_ui_handler->setPlayer(_player);

		if (!grant_starter_items)
			return;

		if (const auto items_it = player_setup.find("starter_items");
			items_it != player_setup.end() && items_it->is_object()) {
			if (const auto equipment_it = items_it->find("equipment");
				equipment_it != items_it->end() && equipment_it->is_array()) {
				for (const auto& starter_item_id : *equipment_it) {
					if (!starter_item_id.is_number_integer())
						continue;
					if (const auto item = _item_database.createItem(starter_item_id.get<int>()))
						_player->equipItem(item);
				}
			}

			if (const auto backpack_it = items_it->find("backpack");
				backpack_it != items_it->end() && backpack_it->is_array()) {
				for (const auto& backpack_item_id : *backpack_it) {
					if (!backpack_item_id.is_number_integer())
						continue;
					if (const auto item = _item_database.createItem(backpack_item_id.get<int>()))
						_player->getBackpack().addItem(item);
				}
			}
		}
	}

	void Engine::startWczoraIntroSequence() {
		if (!_player || !_ui_handler)
			return;

		removeWczoraIntroNpc();
		spawnWczoraIntroCorpse();

		_wczora_intro_slides.clear();
		const auto& config = getWczoraIntroConfig();
		if (config.contains("slides") && config["slides"].is_array()) {
			for (const auto& slide_json : config["slides"]) {
				WczoraIntroSlide slide;
				slide.text = slide_json.value("text", "");
				slide.voice_path = slide_json.value("voice_path", "");
				slide.image_path = slide_json.value("image_path", "");
				slide.duration = slide_json.value("duration", 6.0f);
				_wczora_intro_slides.push_back(std::move(slide));
			}
		}
		for (auto& slide : _wczora_intro_slides) {
			if (!slide.image_path.empty()) {
				slide.image_texture = _resource_manager.getTexture(slide.image_path);
				if (slide.image_texture && slide.image_texture->id > 0)
					SetTextureFilter(*slide.image_texture, TEXTURE_FILTER_TRILINEAR);
			}
		}
		_wczora_intro_slide_index = 0;
		_wczora_intro_phase = WczoraIntroPhase::Slides;
		_wczora_intro_timer = 0.0f;
		_wczora_intro_overlay_alpha = 1.0f;
		_wczora_intro_dialogue_opened = false;
		_wczora_intro_flash_timer = 0.0f;
		_current_camera_zoom = _gameplay_camera_zoom * k_wczora_intro_camera_zoom_factor;
		_camera.resetZoom(_current_camera_zoom);
		if (_wczora_intro_slides.empty()) {
			openWczoraAwakeningDialogue();
			return;
		}
		playWczoraIntroSlideVoice();
		_player->stop();
	}

	void Engine::spawnWczoraIntroCorpse() {
		if (!_player)
			return;

		if (const auto corpse = _wczora_intro_corpse.lock())
			corpse->setDormant(true);

		const float angle = static_cast<float>(GetRandomValue(0, 628)) / 100.0f;
		const float radius = static_cast<float>(GetRandomValue(300, 500)) / 100.0f;
		Vector3 corpse_position = {
			_player->getCenter().x + std::cos(angle) * radius,
			_player->getAltitude(),
			_player->getCenter().y + std::sin(angle) * radius
		};

		if (getCurrentMap() && getCurrentMap()->getNavMesh().isReady())
			corpse_position = getCurrentMap()->getNavMesh().getClosestWalkablePosition(corpse_position);

		auto corpse = std::make_shared<Entity::WandaCorpseNpc>("Zwloki Wandy", corpse_position.x, corpse_position.z, this);
		corpse->setAltitude(corpse_position.y);
		corpse->setAudioManager(&_audio_manager);
		spawnEntity(corpse);
		_wczora_intro_corpse = corpse;
	}

	void Engine::openWczoraAwakeningDialogue() {
		if (!_ui_handler)
			return;

		if (_player) {
			_player->stop();
			const int death_frames = _player->getAnimationFrameCount("Death01");
			_player->setAnimationSpeed(0.0f);
			_player->playAnimation("Death01", false, true, std::max(0, death_frames - 1), true);
		}

		Game::DialogueTree tree = buildSingleNodeDialogueTreeFromJson(getWczoraIntroConfig()["awakening_dialogue"]);

		_wczora_intro_dialogue_opened = true;
		_wczora_intro_phase = WczoraIntroPhase::AwakeningDialogue;
		_wczora_intro_timer = 0.0f;
		_ui_handler->openDialogue(tree, 0, [this](const int, const bool) {
			if (_player) {
				_player->setAnimationSpeed(Entity::Player::DEFAULT_ANIMATION_SPEED);
				_player->playAnimation("LayToIdle", false, true, 0, true);
			}
			if (_quest_manager.startQuest("inspect_wanda_body"))
				_ui_handler->showNotification("Nowy quest: Sprawdz zwloki kobiety", 4.0f);
			_wczora_intro_phase = WczoraIntroPhase::InspectCorpse;
			_wczora_intro_timer = 0.0f;
			_wczora_intro_dialogue_opened = false;
		});
	}

	void Engine::queueWczoraCorpseInspected(const Vector2& corpse_position) {
		_wczora_pending_corpse_completion = true;
		queueWczoraSzeptuchaEncounter(corpse_position);
	}

	void Engine::queueWczoraSzeptuchaEncounter(const Vector2& corpse_position) {
		_wczora_pending_szeptucha_encounter = true;
		_wczora_pending_szeptucha_position = corpse_position;
		_wczora_pending_szeptucha_delay = 0.05f;
	}

	void Engine::startWczoraSzeptuchaEncounter(const Vector2& corpse_position) {
		if (!_player || !_ui_handler)
			return;

		if (_ui_handler->isDialogueOpen()) {
			queueWczoraSzeptuchaEncounter(corpse_position);
			return;
		}

		removeWczoraIntroNpc();

		const auto& szeptucha_config = getWczoraIntroConfig()["szeptucha"];
		const auto& spawn_offset = szeptucha_config["spawn_offset"];
		const Vector2 offset = {
			spawn_offset.value("x", 1.8f),
			spawn_offset.value("y", 0.8f)
		};
		Vector2 spawn_2d = {corpse_position.x + offset.x, corpse_position.y + offset.y};
		Vector3 spawn_position = {spawn_2d.x, _player->getAltitude(), spawn_2d.y};
		if (getCurrentMap() && getCurrentMap()->getNavMesh().isReady())
			spawn_position = getCurrentMap()->getNavMesh().getClosestWalkablePosition(spawn_position);

		auto szeptucha = std::make_shared<Entity::SzeptuchaNpc>("Szeptucha", spawn_position.x, spawn_position.z, this);
		szeptucha->setAltitude(spawn_position.y);
		szeptucha->setAudioManager(&_audio_manager);
		szeptucha->rotateTowardsCenter(_player->getCenter().x, _player->getCenter().y);
		_player->rotateTowardsCenter(szeptucha->getCenter().x, szeptucha->getCenter().y);
		spawnEntity(szeptucha);
		_wczora_intro_npc = szeptucha;
		_wczora_intro_flash_timer = 0.55f;
		_wczora_intro_phase = WczoraIntroPhase::SzeptuchaDialogue;
		_wczora_intro_timer = 0.0f;

		Game::DialogueTree tree = buildLinearDialogueTreeFromJson(
			szeptucha_config["lines"],
			szeptucha_config.value("final_option", "Co?"));

		_ui_handler->openDialogue(tree, 0, [this](const int, const bool) {
			removeWczoraIntroNpc();
			_wczora_intro_flash_timer = 0.55f;
			_wczora_pending_final_dialogue = true;
		});
	}

	void Engine::openWczoraFinalDialogue() {
		if (!_ui_handler)
			return;

		Game::DialogueTree tree = buildSingleNodeDialogueTreeFromJson(getWczoraIntroConfig()["final_dialogue"]);

		_wczora_intro_dialogue_opened = true;
		_wczora_intro_phase = WczoraIntroPhase::FinalDialogue;
		_ui_handler->openDialogue(tree, 0, [this](const int, const bool) {
			finishWczoraIntroSequence();
		});
	}

	void Engine::finishWczoraIntroSequence() {
		stopWczoraIntroSlideVoice();
		_wczora_intro_phase = WczoraIntroPhase::Inactive;
		_wczora_intro_timer = 0.0f;
		_wczora_intro_overlay_alpha = 0.0f;
		_wczora_intro_dialogue_opened = false;
		_wczora_intro_flash_timer = 0.0f;
		_wczora_pending_szeptucha_encounter = false;
		_wczora_pending_final_dialogue = false;
		_wczora_pending_corpse_completion = false;
		_wczora_pending_szeptucha_delay = 0.0f;
	}

	void Engine::removeWczoraIntroNpc() {
		if (const auto npc = _wczora_intro_npc.lock())
			npc->setDormant(true);
		_wczora_intro_npc.reset();
	}

	void Engine::playWczoraIntroSlideVoice() {
		stopWczoraIntroSlideVoice();
		if (_wczora_intro_slide_index >= _wczora_intro_slides.size())
			return;

		const auto& slide = _wczora_intro_slides[_wczora_intro_slide_index];
		if (slide.voice_path.empty())
			return;

		_wczora_intro_slide_voice_id = "intro_slide:" + slide.voice_path;
		_audio_manager.playSoundFile(_wczora_intro_slide_voice_id, slide.voice_path, {1.0f, 1.0f, true});
	}

	void Engine::stopWczoraIntroSlideVoice() {
		if (!_wczora_intro_slide_voice_id.empty())
			_audio_manager.stopSound(_wczora_intro_slide_voice_id);
		_wczora_intro_slide_voice_id.clear();
	}

	bool Engine::isWczoraIntroInteractionOnly() const {
		return _wczora_intro_phase == WczoraIntroPhase::InspectCorpse;
	}

	bool Engine::isWczoraIntroBlockingControl() const {
		return _wczora_intro_phase != WczoraIntroPhase::Inactive &&
			_wczora_intro_phase != WczoraIntroPhase::InspectCorpse;
	}

	void Engine::updateWczoraIntro(const float delta_time) {
		if (_wczora_intro_phase == WczoraIntroPhase::Inactive)
			return;

		if (_wczora_pending_szeptucha_delay > 0.0f)
			_wczora_pending_szeptucha_delay = std::max(0.0f, _wczora_pending_szeptucha_delay - delta_time);

		if (_ui_handler && !_ui_handler->isDialogueOpen()) {
			if (_wczora_pending_szeptucha_encounter) {
				if (_wczora_pending_szeptucha_delay > 0.0f)
					return;

				const Vector2 position = _wczora_pending_szeptucha_position;
				_wczora_pending_szeptucha_encounter = false;
				if (_wczora_pending_corpse_completion) {
					_wczora_pending_corpse_completion = false;
					_quest_manager.completeQuest("inspect_wanda_body", this);
				}
				startWczoraSzeptuchaEncounter(position);
				return;
			}

			if (_wczora_pending_final_dialogue) {
				_wczora_pending_final_dialogue = false;
				openWczoraFinalDialogue();
				return;
			}
		}

		if (_player && _wczora_intro_phase != WczoraIntroPhase::InspectCorpse)
			_player->stop();

		if (_wczora_intro_flash_timer > 0.0f)
			_wczora_intro_flash_timer = std::max(0.0f, _wczora_intro_flash_timer - delta_time);

		_wczora_intro_timer += delta_time;
		switch (_wczora_intro_phase) {
			case WczoraIntroPhase::Slides:
				_wczora_intro_overlay_alpha = 1.0f;
				if (_wczora_intro_slide_index < _wczora_intro_slides.size() &&
					_wczora_intro_timer >= _wczora_intro_slides[_wczora_intro_slide_index].duration) {
					_wczora_intro_slide_index++;
					_wczora_intro_timer = 0.0f;
					if (_wczora_intro_slide_index < _wczora_intro_slides.size()) {
						playWczoraIntroSlideVoice();
					} else {
						stopWczoraIntroSlideVoice();
						if (_player) {
							_player->stop();
							const int death_frames = _player->getAnimationFrameCount("Death01");
							_player->setAnimationSpeed(0.0f);
							_player->playAnimation("Death01", false, true, std::max(0, death_frames - 1), true);
						}
						_wczora_intro_phase = WczoraIntroPhase::FadeFromBlackAfterSlides;
						_wczora_intro_overlay_alpha = 1.0f;
					}
				}
				break;
			case WczoraIntroPhase::FadeFromBlackAfterSlides:
				_wczora_intro_overlay_alpha = std::max(0.38f, 1.0f - _wczora_intro_timer / 2.4f);
				if (_wczora_intro_timer >= 2.4f)
					openWczoraAwakeningDialogue();
				break;
			case WczoraIntroPhase::AwakeningDialogue:
				_wczora_intro_overlay_alpha = 0.38f;
				break;
			case WczoraIntroPhase::InspectCorpse:
				_wczora_intro_overlay_alpha = 0.0f;
				break;
			case WczoraIntroPhase::SzeptuchaDialogue:
				_wczora_intro_overlay_alpha = 0.22f;
				break;
			case WczoraIntroPhase::FinalDialogue:
				_wczora_intro_overlay_alpha = 0.22f;
				break;
			case WczoraIntroPhase::Inactive:
				break;
		}
	}

	void Engine::renderWczoraIntroOverlay() const {
		if (_wczora_intro_phase == WczoraIntroPhase::Inactive && _wczora_intro_flash_timer <= 0.0f)
			return;

		const int width = GetScreenWidth();
		const int height = GetScreenHeight();
		const float alpha = std::clamp(_wczora_intro_overlay_alpha, 0.0f, 1.0f);
		const bool rendering_slide = _wczora_intro_phase == WczoraIntroPhase::Slides &&
			_wczora_intro_slide_index < _wczora_intro_slides.size();
		if (!rendering_slide && alpha > 0.0f)
			DrawRectangle(0, 0, width, height, Fade(BLACK, alpha));

		if (rendering_slide && _ui_handler) {
			const auto& slide = _wczora_intro_slides[_wczora_intro_slide_index];
			const float fade_in = std::clamp(_wczora_intro_timer / 1.2f, 0.0f, 1.0f);
			const float fade_out = std::clamp((slide.duration - _wczora_intro_timer) / 1.2f, 0.0f, 1.0f);
			const float text_alpha = std::min(fade_in, fade_out);
			const float screen_width = static_cast<float>(width);
			const float screen_height = static_cast<float>(height);
			const float image_time = static_cast<float>(GetTime()) + static_cast<float>(_wczora_intro_slide_index) * 13.0f;

			drawAnimatedWczoraIntroImage(slide.image_texture, screen_width, screen_height, image_time);
			DrawRectangleGradientV(0, 0, width, height, UI::withAlpha({30, 14, 10, 255}, 0.10f), UI::withAlpha({5, 5, 8, 255}, 0.55f));
			drawWczoraIntroParticlesFx(screen_width, screen_height, image_time);
			DrawRectangleGradientV(0, 0, width, height, UI::withAlpha(UI::COLOR_ACCENT, 0.02f), UI::withAlpha(BLACK, 0.18f));
			DrawRectangle(0, 0, width, height, Fade(BLACK, 0.22f + (1.0f - text_alpha) * 0.78f));

			const Font& font = _ui_handler->getFont();
			const float font_size = GlobalScaling::scaled(30.0f);
			const float spacing = GlobalScaling::scaled(1.0f);
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

			const float line_height = font_size + GlobalScaling::scaled(10.0f);
			const float total_height = static_cast<float>(lines.size()) * line_height;
			float y = (static_cast<float>(height) - total_height) * 0.5f;
			for (const auto& line : lines) {
				const Vector2 size = MeasureTextEx(font, line.c_str(), font_size, spacing);
				const Vector2 text_position = {(screen_width - size.x) * 0.5f, y};
				DrawTextEx(font, line.c_str(), {text_position.x + 2.0f, text_position.y + 2.0f}, font_size, spacing, Fade(BLACK, text_alpha * 0.72f));
				DrawTextEx(font, line.c_str(), text_position, font_size, spacing, Fade(RAYWHITE, text_alpha));
				y += line_height;
			}
		}

		if (_wczora_intro_flash_timer > 0.0f) {
			const float flash_alpha = std::clamp(_wczora_intro_flash_timer / 0.55f, 0.0f, 1.0f);
			DrawRectangle(0, 0, width, height, Fade(WHITE, flash_alpha));
		}
	}

	void Engine::run() {
		while (isRunning()) {
			const float delta_time = GetFrameTime();
			handleInput();
			update(delta_time);
			render();
		}
	}

	void Engine::handleInput() {
		if (!_ui_handler) return;

		switch (_game_state) {
			case GameState::Menu:
				handleMenuInput();
				break;
			case GameState::GameOver:
				handleGameOverInput();
				break;
			case GameState::SettingsMenu:
				handleSettingsInput();
				break;
			case GameState::LevelSelect:
				handleLevelSelectInput();
				break;
			case GameState::SaveSlotSelect:
				handleSaveSlotSelectInput();
				break;
			case GameState::Playing:
				handlePlayingInput();
				break;
		}
	}

	void Engine::handleMenuInput() {
		const UI::MenuAction action = _ui_handler->handleMenuInput();

		if (action == UI::MenuAction::NewGame) {
			_previous_state = GameState::Menu;
			_pending_new_game_level.clear();
			_ui_handler->openLevelSelect(_level_manager->getRegisteredLevelInfos());
			_game_state = GameState::LevelSelect;
		} else if (action == UI::MenuAction::ContinueGame) {
			loadGameFromSlot(0);
		} else if (action == UI::MenuAction::LoadGame) {
			_previous_state = GameState::Menu;
			_ui_handler->openSaveSlotMenu(_save_game_manager.getSaveSlots(), UI::SaveSlotMenu::Mode::Load);
			_game_state = GameState::SaveSlotSelect;
		} else if (action == UI::MenuAction::Settings) {
			_previous_state = GameState::Menu;
			_ui_handler->openSettings(_settings);
			_game_state = GameState::SettingsMenu;
		} else if (action == UI::MenuAction::Authors) {
			_ui_handler->openAuthors();
		} else if (action == UI::MenuAction::Exit) {
			_is_running = false;
		}
	}

	void Engine::handleGameOverInput() {
		const UI::MenuAction action = _ui_handler->handleGameOverInput();
		if (action == UI::MenuAction::Respawn) {
			_player->respawn();
			_entity_manager->addEntity(_player);
			if (_level_manager && _level_manager->getCurrentLevel())
				_level_manager->getCurrentLevel()->prepareForRespawn(this);
			_game_state = GameState::Playing;
		} else if (action == UI::MenuAction::Exit) {
			_audio_manager.playMusic(MENU_MUSIC_PATH, true, 0.45f);
			_game_state = GameState::Menu;
		}
	}

	void Engine::handleSettingsInput() {
		if (IsKeyPressed(KEY_ESCAPE)) {
			_ui_handler->closeSettingsMenu();
			_game_state = _previous_state;
			_show_pause_menu = (_previous_state == GameState::Playing);
			return;
		}

		const UI::MenuAction action = _ui_handler->handleSettingsInput();
		if (action == UI::MenuAction::Play) {
			_game_state = _previous_state;
			_show_pause_menu = (_previous_state == GameState::Playing);
			return;
		}

		if (_ui_handler->wereSettingsApplied()) {
			applySettings(_ui_handler->getAppliedSettings());
			_ui_handler->closeSettingsMenu();
			_game_state = _previous_state;
			_show_pause_menu = (_previous_state == GameState::Playing);
		}
	}

	void Engine::handleLevelSelectInput() {
		const std::string selected_level = _ui_handler->handleLevelSelectInput();
		if (selected_level.empty())
			return;

		_ui_handler->closeLevelSelect();

		if (selected_level == "BACK") {
			_pending_new_game_level.clear();
			_game_state = GameState::Menu;
			return;
		}

		// Poziomy bez systemu zapisu (np. kreator) startuja od razu, z pomijaniem
		// wyboru slotu i auto-zapisu po zaladowaniu swiata.
		const auto level_infos = _level_manager->getRegisteredLevelInfos();
		const auto level_info = std::ranges::find_if(level_infos, [&](const World::LevelInfo& info) {
			return info.name == selected_level;
		});
		if (level_info != level_infos.end() && !level_info->allows_saves) {
			_pending_new_game_level.clear();
			startNewGame(selected_level, 0);
			return;
		}

		// Po wybraniu poziomu fabularnego prosimy o slot startowy dla nowej gry.
		_pending_new_game_level = selected_level;
		_ui_handler->openSaveSlotMenu(_save_game_manager.getSaveSlots(), UI::SaveSlotMenu::Mode::SelectDefault);
		_game_state = GameState::SaveSlotSelect;
	}

	void Engine::handleSaveSlotSelectInput() {
		const int selected_slot = _ui_handler->handleSaveSlotInput();
		if (selected_slot == 0)
			return;

		const bool opened_from_game = _previous_state == GameState::Playing;
		const UI::SaveSlotMenu::Mode mode = _ui_handler->getSaveSlotMenuMode();
		_ui_handler->closeSaveSlotMenu();

		if (selected_slot < 0) {
			// Anulowanie wyboru slotu w nowej grze cofa nas do wyboru poziomu.
			if (mode == UI::SaveSlotMenu::Mode::SelectDefault) {
				_ui_handler->openLevelSelect(_level_manager->getRegisteredLevelInfos());
				_game_state = GameState::LevelSelect;
				return;
			}

			_game_state = opened_from_game ? GameState::Playing : GameState::Menu;
			_show_pause_menu = opened_from_game;
			return;
		}

		switch (mode) {
			case UI::SaveSlotMenu::Mode::Save:
				saveCurrentGame(selected_slot);
				_game_state = GameState::Playing;
				_show_pause_menu = opened_from_game;
				return;
			case UI::SaveSlotMenu::Mode::SelectDefault:
				startNewGame(_pending_new_game_level, selected_slot);
				_pending_new_game_level.clear();
				return;
			case UI::SaveSlotMenu::Mode::Load:
			default:
				loadGameFromSlot(selected_slot);
				return;
		}
	}

	void Engine::startNewGame(const std::string& level_name, const int default_slot) {
		_save_game_manager.clearActiveSlot();
		_show_pause_menu = false;
		_previous_state = GameState::Menu;
		_has_pending_save = false;
		_pending_save_state = {};

		_boss_manager.resetRuntimeState(this);
		_boss_manager.clearDefeatedBosses();
		_quest_manager.resetAll();
		createFreshPlayer(level_name != "Wczora");

		queueLevelLoad(level_name, "", true, default_slot);
	}

	bool Engine::saveCurrentGame(const int slot) {
		const bool saved = _save_game_manager.saveGame(*this, slot);
		if (_ui_handler)
			_ui_handler->showNotification(saved ? "Gra zapisana." : "Nie udalo sie zapisac gry.", 3.0f);

		return saved;
	}

	bool Engine::saveGameToActiveSlot() {
		const int active_slot = _save_game_manager.getActiveSlot();
		if (active_slot <= 0)
			return false;

		return saveCurrentGame(active_slot);
	}

	bool Engine::loadGameFromSlot(const int slot) {
		if (slot == 0 && !_save_game_manager.hasAnySave()) {
			if (_ui_handler)
				_ui_handler->showNotification("Brak zapisu do wczytania.", 3.0f);
			return false;
		}

		nlohmann::json save_state;
		int resolved_slot = 0;
		if (!_save_game_manager.tryReadSave(slot, save_state, resolved_slot)) {
			if (_ui_handler)
				_ui_handler->showNotification("Nie udalo sie wczytac zapisu.", 3.0f);
			return false;
		}

		const std::string current_level_name = save_state.value("current_level", "");
		if (current_level_name.empty()) {
			if (_ui_handler)
				_ui_handler->showNotification("Nie udalo sie wczytac zapisu.", 3.0f);
			return false;
		}

		_show_pause_menu = false;
		_previous_state = GameState::Menu;
		_has_pending_save = true;
		_pending_save_state = std::move(save_state);
		_pending_save_slot = resolved_slot;

		_boss_manager.resetRuntimeState(this);
		_boss_manager.clearDefeatedBosses();
		_quest_manager.resetAll();
		createFreshPlayer(false);

		const std::string initial_location = _pending_save_state.value("current_location", "");
		queueLevelLoad(current_level_name, initial_location, false, 0);
		return true;
	}

	void Engine::handlePlayingInput() {
		if (isWczoraIntroBlockingControl()) {
			_show_pause_menu = false;
			if (_player)
				_player->stop();
			if (_ui_handler && _ui_handler->isDialogueOpen())
				_ui_handler->handleInput();
			return;
		}

		if (!isWczoraIntroInteractionOnly() && IsKeyPressed(KEY_ESCAPE)) {
			if (_ui_handler->closeOpenWindows())
				return;

			_show_pause_menu = !_show_pause_menu;
			return;
		}

		if (!isWczoraIntroInteractionOnly() && _show_pause_menu) {
			const auto* current_level = _level_manager ? _level_manager->getCurrentLevel() : nullptr;
			const bool saves_enabled = current_level && current_level->allowsSaves();

			const UI::MenuAction action = _ui_handler->handlePauseMenuInput(saves_enabled);
			if (action == UI::MenuAction::Play) {
				_show_pause_menu = false;
			} else if (action == UI::MenuAction::SaveGame) {
				_previous_state = GameState::Playing;
				_ui_handler->openSaveSlotMenu(_save_game_manager.getSaveSlots(), UI::SaveSlotMenu::Mode::Save);
				_game_state = GameState::SaveSlotSelect;
				_show_pause_menu = false;
			} else if (action == UI::MenuAction::LoadGame) {
				_previous_state = GameState::Playing;
				_ui_handler->openSaveSlotMenu(_save_game_manager.getSaveSlots(), UI::SaveSlotMenu::Mode::Load);
				_game_state = GameState::SaveSlotSelect;
				_show_pause_menu = false;
			} else if (action == UI::MenuAction::Settings) {
				_previous_state = GameState::Playing;
				_ui_handler->openSettings(_settings);
				_game_state = GameState::SettingsMenu;
				_show_pause_menu = false;
			} else if (action == UI::MenuAction::MainMenu || action == UI::MenuAction::Exit) {
				_audio_manager.playMusic(MENU_MUSIC_PATH, true, 1.f);
				_game_state = GameState::Menu;
				_show_pause_menu = false;
			}
			return;
		}

		const bool dialogue_was_open = _ui_handler->isDialogueOpen();
		_ui_handler->handleInput();
		if (dialogue_was_open || isWczoraIntroBlockingControl())
			return;

		const auto dev_level = dynamic_cast<World::DevLevel*>(_level_manager->getCurrentLevel());
		if (dev_level)
			_camera.handleInput();

		const Vector2 mouse_pos = GetMousePosition();
		const float cursor_plane_height = _player ? _player->getAltitude() : 0.0f;
		const Vector2 fallback = screenToWorldAtHeight(_camera.get(), mouse_pos.x, mouse_pos.y, cursor_plane_height);
		Vector3 mouse_world_pos = {fallback.x, cursor_plane_height, fallback.y};
		const bool needs_precise_ground_hit =
			IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
			IsKeyPressed(KEY_Q) ||
			IsKeyPressed(KEY_W) ||
			IsKeyPressed(KEY_E) ||
			IsKeyPressed(KEY_R);

		if (needs_precise_ground_hit && getCurrentMap()) {
			const Ray ray = GetMouseRay(mouse_pos, _camera.get());
			const RayCollision collision = getCurrentMap()->getRayCollision(ray);
			if (collision.hit)
				mouse_world_pos = collision.point;
		}

		_hover_update_timer -= GetFrameTime();
		const float hover_dx = mouse_pos.x - _last_hover_mouse_pos.x;
		const float hover_dy = mouse_pos.y - _last_hover_mouse_pos.y;
		const bool hover_mouse_moved = hover_dx * hover_dx + hover_dy * hover_dy >= k_hover_mouse_move_threshold_sq;
		if (hover_mouse_moved || _hover_update_timer <= 0.0f) {
			_entity_manager->updateHoverState(mouse_pos.x, mouse_pos.y, _camera.get());
			_last_hover_mouse_pos = mouse_pos;
			_hover_update_timer = k_hover_update_interval;
		}

		_level_manager->handleInput(this);
		if (dev_level && dev_level->isTyping())
			return;

		if (!isWczoraIntroInteractionOnly() && !_ui_handler->isInputBlocked() && IsKeyPressed(KEY_ONE) && _player)
			(void)_player->startConsumeFood();

		if (_controller) {
			if (isWczoraIntroInteractionOnly())
				_controller->handleInteractionOnly(mouse_world_pos, mouse_pos.x, mouse_pos.y);
			else
				_controller->handleInput(mouse_world_pos, mouse_pos.x, mouse_pos.y);
		}
	}

	void Engine::update(const float delta_time) {
		_audio_manager.update();

		if (_game_state == GameState::Loading) {
			processLoading();
			return;
		}

		if (_game_state == GameState::Menu ||
			_game_state == GameState::SettingsMenu ||
			_game_state == GameState::LevelSelect ||
			_game_state == GameState::SaveSlotSelect) {
			if (_ui_handler) _ui_handler->update(delta_time);
			return;
		}

		if (!_player || !_entity_manager)
			return;

		if (_player->isDead()) {
			if (_boss_manager.isFightActive()) {
				_boss_manager.endBossFight(false, this);
			}
			_game_state = GameState::GameOver;
			return;
		}

		if (!dynamic_cast<World::DevLevel*>(_level_manager->getCurrentLevel())) {
			const float target_zoom = (_wczora_intro_phase != WczoraIntroPhase::Inactive)
				? _gameplay_camera_zoom * k_wczora_intro_camera_zoom_factor
				: _gameplay_camera_zoom;
			if (_wczora_intro_phase != WczoraIntroPhase::Inactive) {
				_current_camera_zoom = target_zoom;
			} else {
				const float zoom_t = std::clamp(delta_time * k_camera_zoom_return_speed, 0.0f, 1.0f);
				_current_camera_zoom += (target_zoom - _current_camera_zoom) * zoom_t;
			}
			_camera.resetZoom(_current_camera_zoom);
		}

		_camera.follow(_player.get());
		_lighting_system.update(_camera.get());
		if (_ui_handler) _ui_handler->update(delta_time);
		updateWczoraIntro(delta_time);
		if (_wczora_intro_phase != WczoraIntroPhase::Inactive && !isWczoraIntroInteractionOnly()) {
			_entity_manager->updateEntities(delta_time);
			collectPendingSpawns();
			return;
		}
		_level_manager->update(this, delta_time);
		_controller->update(delta_time);

		_entity_manager->updateEntities(delta_time);
		_entity_manager->handleEntitiesCollisions();
		_quest_manager.update(this);
		_boss_manager.update(this, delta_time);
		collectPendingSpawns();
	}

	void Engine::collectPendingSpawns() {
		std::vector<std::shared_ptr<Entity::Entity>> new_spawns;
		for (const auto& entity : _entity_manager->getEntities()) {
			const auto& spawns = entity->getPendingSpawns();
			if (!spawns.empty()) {
				new_spawns.insert(new_spawns.end(), spawns.begin(), spawns.end());
				entity->clearPendingSpawns();
			}
		}

		for (const auto& spawn : new_spawns)
			spawnEntity(spawn);
	}

	void Engine::loadGameplaySounds() {
		_audio_manager.loadSound(Audio::SoundId::ZombieScream, Audio::SoundPath::ZombieScream);
		_audio_manager.loadSound(Audio::SoundId::ZombieDeath, Audio::SoundPath::ZombieDeath);
		_audio_manager.loadSound(Audio::SoundId::ZombieAmbient, Audio::SoundPath::ZombieAmbient);
		_audio_manager.loadSound(Audio::SoundId::SwordSlash, Audio::SoundPath::SwordSlash);
		_audio_manager.loadSound(Audio::SoundId::FireballCast, Audio::SoundPath::FireballCast);
		_audio_manager.loadSound(Audio::SoundId::DevilDeath, Audio::SoundPath::DevilDeath);
		_audio_manager.loadSound(Audio::SoundId::DevilDash, Audio::SoundPath::DevilDash);
		_audio_manager.loadSound(Audio::SoundId::DevilAggro, Audio::SoundPath::DevilAggro);
		_audio_manager.loadSound(Audio::SoundId::DevilPunch, Audio::SoundPath::DevilPunch);
		_audio_manager.loadSound(Audio::SoundId::DevilStep, Audio::SoundPath::DevilStep);
		_audio_manager.loadSound(Audio::SoundId::ChestOpen, Audio::SoundPath::ChestOpen);
		_audio_manager.loadSound(Audio::SoundId::DevilDashHit, Audio::SoundPath::DevilDashHit);
		_audio_manager.loadSound(Audio::SoundId::ItemEquip, Audio::SoundPath::ItemEquip);
		_audio_manager.loadSound(Audio::SoundId::FireballHit, Audio::SoundPath::FireballHit);
		_audio_manager.loadSound(Audio::SoundId::PlayerHurt, Audio::SoundPath::PlayerHurt);
		_audio_manager.loadSound(Audio::SoundId::HumanDeath, Audio::SoundPath::HumanDeath);
		_audio_manager.loadSound(Audio::SoundId::KnifeThrow, Audio::SoundPath::KnifeThrow);
		_audio_manager.loadSound(Audio::SoundId::CatMeow, Audio::SoundPath::CatMeow);
		_audio_manager.loadSound(Audio::SoundId::MiniMushroomAttack, Audio::SoundPath::MiniMushroomAttack);
		_audio_manager.loadSound(Audio::SoundId::MiniMushroomWormExit, Audio::SoundPath::MiniMushroomWormExit);
		_audio_manager.loadSound(Audio::SoundId::PlayerEatSupplies, Audio::SoundPath::PlayerEatSupplies);
	}

	void Engine::render() const {
		BeginDrawing();
		ClearBackground(Color{30, 30, 35, 255});

		if (_game_state == GameState::Loading) {
			UI::LoadingScreen::render(_loading_progress, _loading_status, _loading_title);
			EndDrawing();
			return;
		}

		if (_game_state == GameState::Menu && _ui_handler) {
			_ui_handler->renderMainMenu();
		} else if (_game_state == GameState::SettingsMenu && _ui_handler) {
			_ui_handler->renderMainMenu();
			_ui_handler->renderSettingsMenu();
		} else if (_game_state == GameState::LevelSelect && _ui_handler) {
			_ui_handler->renderMainMenu();
			_ui_handler->renderLevelSelectMenu();
		} else if (_game_state == GameState::SaveSlotSelect && _ui_handler) {
			// Wybor slotu z menu glownego to osobny ekran - rysujemy samo tlo
			// menu zamiast przyciskow "Nowa gra" itp., zeby nie przebijaly sie
			// spod polprzezroczystego dimm-u SaveSlotMenu. Z pauzy zachowujemy
			// widoczna rozgrywke w tle.
			if (_previous_state == GameState::Playing)
				renderGameplay();
			else
				_ui_handler->drawSharedMenuBackground();

			_ui_handler->renderSaveSlotMenu();
		} else if (_game_state == GameState::GameOver) {
			renderWorld();
			if (_ui_handler) _ui_handler->renderGameOverScreen();
		} else {
			renderGameplay();
		}

		EndDrawing();
	}

	void Engine::renderWorld() const {
		if (!getCurrentMap() || !_player || !_entity_manager)
			return;

		BeginMode3D(_camera.get());

		_lighting_system.applyToModel(getCurrentMap()->getModel());

		getCurrentMap()->render(_camera.get());
		_entity_manager->renderEntities(_camera.get());

		EndMode3D();
	}

	void Engine::renderGameplay() const {
		if (!getCurrentMap() || !_player || !_entity_manager)
			return;

		renderWorld();
		renderGameplayVignetteOverlay();
		renderWczoraIntroOverlay();

		if (_wczora_intro_phase != WczoraIntroPhase::Inactive && !isWczoraIntroInteractionOnly()) {
			if (_ui_handler)
				_ui_handler->renderDialogueOnly();
			return;
		}

		if (_ui_handler) _ui_handler->render(_camera, &_boss_manager);

		if (_show_pause_menu && _ui_handler) {
			const auto* current_level = _level_manager ? _level_manager->getCurrentLevel() : nullptr;
			_ui_handler->renderPauseMenu(current_level && current_level->allowsSaves());
		}

		_level_manager->renderUI(const_cast<Engine*>(this));
	}

	void Engine::renderGameplayVignetteOverlay() const {
		const int width = GetScreenWidth();
		const int height = GetScreenHeight();
		const int edge_x = static_cast<int>(static_cast<float>(width) * 0.18f);
		const int edge_y = static_cast<int>(static_cast<float>(height) * 0.18f);

		DrawRectangleGradientH(0, 0, edge_x, height, Fade(BLACK, 0.24f), Fade(BLACK, 0.0f));
		DrawRectangleGradientH(width - edge_x, 0, edge_x, height, Fade(BLACK, 0.0f), Fade(BLACK, 0.24f));
		DrawRectangleGradientV(0, 0, width, edge_y, Fade(BLACK, 0.20f), Fade(BLACK, 0.0f));
		DrawRectangleGradientV(0, height - edge_y, width, edge_y, Fade(BLACK, 0.0f), Fade(BLACK, 0.22f));
	}

	void Engine::applySettings(const Settings& new_settings) {
		_settings = new_settings;

		if (IsWindowFullscreen()) {
			if (!_settings.fullscreen)
				ToggleFullscreen();
			else
				SetWindowSize(_settings.resolution.width, _settings.resolution.height);
		} else if (_settings.fullscreen) {
			SetWindowSize(_settings.resolution.width, _settings.resolution.height);
			ToggleFullscreen();
		} else {
			SetWindowSize(_settings.resolution.width, _settings.resolution.height);
		}

		GlobalScaling::setManualScale(_settings.ui_scale);
		_audio_manager.setMasterVolume(_settings.master_volume);
		_audio_manager.setMusicVolume(_settings.music_volume);
		_audio_manager.setEffectsVolume(_settings.effects_volume);

		if (_settings.save())
			Logger::debugLog("Zapisano ustawienia.");
	}

} // namespace Nawia::Core
