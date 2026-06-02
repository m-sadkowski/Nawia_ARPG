#include "DevLevel.h"

#include <Engine.h>
#include <Entity.h>
#include <EntityFactory.h>
#include <ItemDatabase.h>
#include <LocationJsonUtils.h>
#include <Logger.h>
#include <Map.h>
#include <MathUtils.h>
#include <Player.h>
#include <UIHandler.h>

#include <json.hpp>
#include <raymath.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

using json = nlohmann::json;

namespace Nawia::World {

	namespace {

		constexpr const char* PLACEHOLDER_MODEL = "placeholder";
		constexpr const char* DEFAULT_LIGHTING_FILE = "assets/maps/forest_lighting.json";
		constexpr float LIGHT_MOVE_STEP = 1.0f;
		constexpr int PRIMARY_LIGHT_INDEX = 0;
		constexpr int OVERLAY_MARGIN = 10;
		constexpr float MAP_SCALE_MIN = 0.05f;
		constexpr float MAP_SCALE_MAX = 5.0f;
		constexpr float NAVMESH_HEIGHT_MIN = -15.0f;
		constexpr float NAVMESH_HEIGHT_MAX = 5.0f;
		constexpr float NAV_BLOCKER_SIZE_MIN = 0.5f;
		constexpr float NAV_BLOCKER_SIZE_MAX = 80.0f;
		constexpr float NAV_BLOCKER_RADIUS_MIN = 0.25f;
		constexpr float NAV_BLOCKER_RADIUS_MAX = 40.0f;
		constexpr float NAV_BLOCKER_HEIGHT_MIN = -15.0f;
		constexpr float NAV_BLOCKER_HEIGHT_MAX = 12.0f;
		constexpr float WATER_PLANE_HALF_SIZE = 90.0f;
		constexpr int MAX_VISIBLE_ITEMS = 11;

		constexpr Rectangle LEFT_PANEL = {20.0f, 20.0f, 330.0f, 430.0f};
		constexpr Rectangle CENTER_PANEL = {0.0f, 20.0f, 360.0f, 310.0f};
		constexpr Rectangle RIGHT_PANEL = {0.0f, 20.0f, 280.0f, 720.0f};

		const std::vector<std::string> PLAYER_ANIMATION_OPTIONS = {
			"A_TPose",
			"Chest_Open",
			"ClimbUp_1m_RM",
			"Consume",
			"Farm_Harvest",
			"Farm_PlantSeed",
			"Farm_Watering",
			"Hit_Knockback",
			"Hit_Knockback_RM",
			"Idle_FoldArms_Loop",
			"Idle_Lantern_Loop",
			"Idle_No_Loop",
			"Idle_Rail_Call",
			"Idle_Rail_Loop",
			"Idle_Shield_Break",
			"Idle_Shield_Loop",
			"Idle_TalkingPhone_Loop",
			"LayToIdle",
			"Melee_Hook",
			"Melee_Hook_Rec",
			"NinjaJump_Idle_Loop",
			"NinjaJump_Land",
			"NinjaJump_Start",
			"OverhandThrow",
			"Shield_Dash_RM",
			"Shield_OneShot",
			"Slide_Exit",
			"Slide_Loop",
			"Slide_Start",
			"Sword_Block",
			"Sword_Dash_RM",
			"Sword_Regular_A",
			"Sword_Regular_A_Rec",
			"Sword_Regular_B",
			"Sword_Regular_B_Rec",
			"Sword_Regular_C",
			"Sword_Regular_Combo",
			"TreeChopping_Loop",
			"Walk_Carry_Loop",
			"Yes",
			"Zombie_Idle_Loop",
			"Zombie_Scratch",
			"Zombie_Walk_Fwd_Loop",
			"Crouch_Fwd_Loop",
			"Crouch_Idle_Loop",
			"Dance_Loop",
			"Death01",
			"Driving_Loop",
			"Fixing_Kneeling",
			"Hit_Chest",
			"Hit_Head",
			"Idle_Loop",
			"Idle_Talking_Loop",
			"Idle_Torch_Loop",
			"Interact",
			"Jog_Fwd_Loop",
			"Jump_Land",
			"Jump_Loop",
			"Jump_Start",
			"PickUp_Table",
			"Pistol_Aim_Down",
			"Pistol_Aim_Neutral",
			"Pistol_Aim_Up",
			"Pistol_Idle_Loop",
			"Pistol_Reload",
			"Pistol_Shoot",
			"Punch_Cross",
			"Punch_Jab",
			"Push_Loop",
			"Roll",
			"Roll_RM",
			"Sitting_Enter",
			"Sitting_Exit",
			"Sitting_Idle_Loop",
			"Sitting_Talking_Loop",
			"Spell_Simple_Enter",
			"Spell_Simple_Exit",
			"Spell_Simple_Idle_Loop",
			"Spell_Simple_Shoot",
			"Sprint_Loop",
			"Swim_Fwd_Loop",
			"Swim_Idle_Loop",
			"Sword_Attack",
			"Sword_Attack_RM",
			"Sword_Idle",
			"Walk_Formal_Loop",
			"Walk_Loop"
		};

		std::filesystem::path resolveAssetPath(const std::filesystem::path& relative_asset_path) {
			return (std::filesystem::path("assets") / relative_asset_path).lexically_normal();
		}

		std::string toPathString(const std::filesystem::path& path) {
			return LocationJsonUtils::toPathString(path);
		}

		std::string formatFloat(const float value, const int precision = 2) {
			std::ostringstream stream;
			stream << std::fixed << std::setprecision(precision) << value;
			return stream.str();
		}

		std::string displayNameFromStem(std::string stem) {
			return LocationJsonUtils::displayNameFromStem(std::move(stem));
		}

		std::string slugify(std::string text) {
			std::string result;
			result.reserve(text.size());

			for (char c : text) {
				const unsigned char uc = static_cast<unsigned char>(c);
				if (std::isalnum(uc)) {
					result.push_back(static_cast<char>(std::tolower(uc)));
				} else if (std::isspace(uc) || c == '-' || c == '_') {
					if (result.empty() || result.back() != '_')
						result.push_back('_');
				}
			}

			while (!result.empty() && result.back() == '_')
				result.pop_back();

			return result.empty() ? "nowa_lokacja" : result;
		}

		std::string sanitizeJsonFilename(const std::string& input, const std::string& fallback_stem) {
			std::filesystem::path path(input.empty() ? fallback_stem : input);
			std::string filename = path.filename().string();
			if (filename.empty())
				filename = fallback_stem;

			std::string sanitized;
			sanitized.reserve(filename.size());
			for (char c : filename) {
				const unsigned char uc = static_cast<unsigned char>(c);
				if (std::isalnum(uc) || c == '_' || c == '-' || c == '.') {
					sanitized.push_back(c);
				} else if (std::isspace(uc)) {
					sanitized.push_back('_');
				}
			}

			if (sanitized.empty())
				sanitized = fallback_stem;

			if (std::filesystem::path(sanitized).extension() != ".json")
				sanitized += ".json";

			return sanitized;
		}

		bool readJsonFile(const std::filesystem::path& path, json& output) {
			return LocationJsonUtils::readJsonFile(path, output, "DevLevel");
		}

		bool tryParseFloat(const std::string& text, float& output) {
			try {
				size_t parsed_chars = 0;
				output = std::stof(text, &parsed_chars);
				return parsed_chars > 0;
			} catch (const std::exception&) {
				return false;
			}
		}

		bool tryParseInt(const std::string& text, int& output) {
			try {
				size_t parsed_chars = 0;
				output = std::stoi(text, &parsed_chars);
				return parsed_chars > 0;
			} catch (const std::exception&) {
				return false;
			}
		}

		Vector2 parseVector2(const json& data, const Vector2 fallback = {0.0f, 0.0f}) {
			return LocationJsonUtils::parseVector2(data, fallback);
		}

		Vector3 parseVector3(const json& data, const Vector3 fallback = {0.0f, 0.0f, 0.0f}) {
			return LocationJsonUtils::parseVector3(data, fallback);
		}

		json vector2ToJson(const Vector2 value) {
			return LocationJsonUtils::vector2ToJson(value);
		}

		json vector3ToJson(const Vector3 value) {
			return LocationJsonUtils::vector3ToJson(value);
		}

		bool isEnemyType(const std::string& type) {
			return type == "devil" ||
				   type == "bandit" ||
				   type == "walking_dead" ||
				   type == "frog" ||
				   type == "worm" ||
				   type == "mini_mushroom_infected";
		}

		std::string categoryFromEntityType(const std::string& type) {
			if (isEnemyType(type)) return "spawners";
			if (type == "chest") return "chests";
			if (type == "npc") return "npcs";
			if (type == "static_object") return "props";
			if (type == "mini_mushroom_prop") return "props";
			if (type == "teleport") return "teleports";
			if (type == "checkpoint") return "checkpoints";
			if (type == "checkpoint_mushroom_npc") return "checkpoints";
			if (type == "boss_trigger") return "boss_triggers";
			if (type == "nav_blocker") return "nav_blockers";
			return "props";
		}

		Color getCategoryColor(const std::string& category) {
			if (category == "spawners") return RED;
			if (category == "chests") return GOLD;
			if (category == "npcs") return SKYBLUE;
			if (category == "props") return GREEN;
			if (category == "teleports") return PURPLE;
			if (category == "checkpoints") return LIME;
			if (category == "boss_triggers") return MAGENTA;
			if (category == "nav_blockers") return BLUE;
			return GRAY;
		}

		NavMeshBlockerShape navBlockerShapeFromString(const std::string& shape) {
			return shape == "circle" ? NavMeshBlockerShape::Circle : NavMeshBlockerShape::Box;
		}

		void drawDevText(const Font& font, const char* text, const float x, const float y, const float size, const Color color) {
			DrawTextEx(font, text, {x, y}, size, 1.0f, color);
		}

		bool drawButton(const Font& font, const char* text, const int x, const int y, const int width, const int height, const Color base_color) {
			const Rectangle rect = {
				static_cast<float>(x),
				static_cast<float>(y),
				static_cast<float>(width),
				static_cast<float>(height),
			};
			const bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);

			DrawRectangleRec(rect, hovered ? ColorAlpha(base_color, 0.82f) : base_color);
			DrawRectangleLinesEx(rect, 2, ColorAlpha(RAYWHITE, 0.55f));

			const Vector2 text_size = MeasureTextEx(font, text, 20, 1.0f);
			drawDevText(
				font,
				text,
				x + width / 2.0f - text_size.x / 2.0f,
				y + height / 2.0f - text_size.y / 2.0f,
				20,
				RAYWHITE
			);

			return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
		}

		void drawLabel(const Font& font, const char* text, const int x, const int y) {
			drawDevText(font, text, static_cast<float>(x), static_cast<float>(y), 17, LIGHTGRAY);
		}

		bool drawTextInput(
			const Font& font,
			std::string& buffer,
			const int x,
			const int y,
			const int width,
			const int height,
			const bool active
		) {
			const Rectangle rect = {
				static_cast<float>(x),
				static_cast<float>(y),
				static_cast<float>(width),
				static_cast<float>(height),
			};

			DrawRectangleRec(rect, active ? Color{35, 42, 38, 245} : Color{8, 10, 9, 235});
			DrawRectangleLinesEx(rect, 2, active ? ORANGE : ColorAlpha(RAYWHITE, 0.45f));

			BeginScissorMode(x + 4, y + 1, std::max(1, width - 8), std::max(1, height - 2));
			drawDevText(font, buffer.c_str(), x + 6.0f, y + height / 2.0f - 10.0f, 20, RAYWHITE);
			EndScissorMode();

			if (active) {
				int key = GetCharPressed();
				while (key > 0) {
					if (key >= 32 && key <= 125)
						buffer += static_cast<char>(key);
					key = GetCharPressed();
				}

				if (IsKeyPressed(KEY_BACKSPACE) && !buffer.empty())
					buffer.pop_back();
			}

			return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rect);
		}

		int drawDropdown(
			const Font& font,
			const Rectangle rect,
			const std::vector<std::string>& options,
			const int selected_index,
			bool& open,
			const int max_visible_items
		) {
			const bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
			DrawRectangleRec(rect, hovered ? Color{245, 245, 245, 255} : RAYWHITE);
			DrawRectangleLinesEx(rect, 2, BLACK);

			const std::string selected_text =
				(selected_index >= 0 && selected_index < static_cast<int>(options.size())) ? options[selected_index] : "Brak";

			BeginScissorMode(
				static_cast<int>(rect.x + 6.0f),
				static_cast<int>(rect.y + 1.0f),
				static_cast<int>(rect.width - 38.0f),
				static_cast<int>(rect.height - 2.0f)
			);
			drawDevText(font, selected_text.c_str(), rect.x + 8.0f, rect.y + rect.height / 2.0f - 11.0f, 22, BLACK);
			EndScissorMode();

			drawDevText(font, "v", rect.x + rect.width - 28.0f, rect.y + rect.height / 2.0f - 14.0f, 28, BLACK);

			if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				open = !open;
				return -1;
			}

			return -1;
		}

		int drawDropdownOptions(
			const Font& font,
			const Rectangle rect,
			const std::vector<std::string>& options,
			bool& open,
			const int max_visible_items
		) {
			if (!open)
				return -1;

			const int visible_items = std::min(static_cast<int>(options.size()), max_visible_items);
			for (int index = 0; index < visible_items; ++index) {
				const Rectangle option_rect = {
					rect.x,
					rect.y + rect.height * static_cast<float>(index + 1),
					rect.width,
					rect.height,
				};
				const bool option_hovered = CheckCollisionPointRec(GetMousePosition(), option_rect);

				DrawRectangleRec(option_rect, option_hovered ? Color{220, 235, 210, 255} : RAYWHITE);
				DrawRectangleLinesEx(option_rect, 1, BLACK);

				BeginScissorMode(
					static_cast<int>(option_rect.x + 6.0f),
					static_cast<int>(option_rect.y + 1.0f),
					static_cast<int>(option_rect.width - 12.0f),
					static_cast<int>(option_rect.height - 2.0f)
				);
				drawDevText(font, options[index].c_str(), option_rect.x + 8.0f, option_rect.y + 9.0f, 19, BLACK);
				EndScissorMode();

				if (option_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
					open = false;
					return index;
				}
			}

			return -1;
		}

		bool drawSlider(
			const Font& font,
			const Rectangle rect,
			float& value,
			const float min_value,
			const float max_value,
			EditorTextField& active_field,
			const EditorTextField field,
			const Color knob_color
		) {
			const Vector2 mouse = GetMousePosition();
			const Rectangle hitbox = {rect.x - 8.0f, rect.y - 9.0f, rect.width + 16.0f, rect.height + 18.0f};
			const bool hovered = CheckCollisionPointRec(mouse, hitbox);

			bool changed = false;
			if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
				active_field = field;

			if (active_field == field && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				const float t = std::clamp((mouse.x - rect.x) / rect.width, 0.0f, 1.0f);
				const float new_value = min_value + (max_value - min_value) * t;
				if (std::abs(new_value - value) > 0.0001f) {
					value = new_value;
					changed = true;
				}
			}

			if (active_field == field && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
				active_field = EditorTextField::None;

			const float t = std::clamp((value - min_value) / (max_value - min_value), 0.0f, 1.0f);
			DrawRectangleRec(rect, Fade(GRAY, 0.65f));
			DrawRectangleLinesEx(rect, 1, LIGHTGRAY);
			DrawRectangle(
				static_cast<int>(rect.x),
				static_cast<int>(rect.y),
				static_cast<int>(rect.width * t),
				static_cast<int>(rect.height),
				Fade(knob_color, 0.35f)
			);
			DrawCircle(
				static_cast<int>(rect.x + rect.width * t),
				static_cast<int>(rect.y + rect.height * 0.5f),
				active_field == field ? 10.0f : 8.0f,
				hovered || active_field == field ? ORANGE : knob_color
			);

			const std::string value_text = formatFloat(value, 2);
			const Vector2 text_size = MeasureTextEx(font, value_text.c_str(), 17, 1.0f);
			drawDevText(font, value_text.c_str(), rect.x + rect.width - text_size.x, rect.y + rect.height + 5.0f, 17, RAYWHITE);

			return changed;
		}

		float readNavmeshMinHeight(const json& root, const float fallback) {
			const auto navmesh_it = root.find("navmesh");
			if (navmesh_it == root.end() || !navmesh_it->is_object())
				return fallback;

			return navmesh_it->value("min_walkable_height", fallback);
		}

		float readCameraZoom(const json& root, const float fallback) {
			const auto camera_it = root.find("camera");
			if (camera_it != root.end() && camera_it->is_object())
				return camera_it->value("zoom", fallback);

			if (root.contains("camera_zoom") && root["camera_zoom"].is_number())
				return root["camera_zoom"].get<float>();

			return fallback;
		}

	} // namespace

	std::vector<std::string> DevLevel::getLocations() const {
		return {_active_location_name.empty() ? "Nowa lokacja" : _active_location_name};
	}

	bool DevLevel::isTyping() const {
		return _current_mode != EditorMode::None ||
			   isTextFieldActive() ||
			   isAnyDropdownOpen() ||
			   isMouseOverEditorUI();
	}

	void DevLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie DevLevel jako kreatora poziomow...");
		Entity::Entity::DebugColliders = true;

		_map = std::make_unique<Core::Map>(engine->getResourceManager());

		loadAvailableMapModels();
		loadAvailableLocations();
		loadAvailableBossIds();

		engine->getLightingSystem().loadLightingFromJson(DEFAULT_LIGHTING_FILE);
		engine->getEntityManager().clearNonPlayerEntities();

		initializeNewLocation(engine);
	}

	void DevLevel::onExit(Core::Engine* engine) {
		Entity::Entity::DebugColliders = false;
		Level::onExit(engine);
	}

	void DevLevel::handleInput(Core::Engine* engine) {
		if (_current_mode != EditorMode::None) {
			handleUIInput(engine);
			return;
		}

		if (IsKeyPressed(KEY_ESCAPE)) {
			_active_text_field = EditorTextField::None;
			_map_dropdown_open = false;
			_location_dropdown_open = false;
			return;
		}

		if (isTextFieldActive() || isAnyDropdownOpen())
			return;

		if (!_is_testing_level && (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_X))) {
			deleteNearestObject(engine);
			return;
		}

		handleEditingInput(engine);
	}

	void DevLevel::update(Core::Engine* engine, float dt) {
		if (engine)
			_camera_zoom = engine->getCamera().getZoomFactor();

		if (const auto player = engine->getPlayer()) {
			float movement_speed = 5.0f;
			if (IsKeyDown(KEY_LEFT_SHIFT))
				movement_speed = 25.0f;

			player->setMovementSpeed(movement_speed);

			if (_current_mode == EditorMode::None && !isTextFieldActive()) {
				Vector2 movement = {0.0f, 0.0f};
				if (IsKeyDown(KEY_W)) movement.y -= 1.0f;
				if (IsKeyDown(KEY_S)) movement.y += 1.0f;
				if (IsKeyDown(KEY_A)) movement.x -= 1.0f;
				if (IsKeyDown(KEY_D)) movement.x += 1.0f;

				if (movement.x != 0.0f || movement.y != 0.0f) {
					movement = Vector2Normalize(movement);
					player->setX(player->getX() + movement.x * movement_speed * dt);
					player->setY(player->getY() + movement.y * movement_speed * dt);
				}
			}
		}

		Level::update(engine, dt);
	}

	void DevLevel::renderUI(Core::Engine* engine) {
		if (!engine || engine->isPaused())
			return;

		if (!_is_testing_level)
			renderPlacedObjects(engine);

		renderWaterCutoffPlane(*engine);
		renderEditorHud(engine);

		if (_current_mode == EditorMode::None)
			return;

		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.55f));

		switch (_current_mode) {
			case EditorMode::SpawnerType:
				renderSpawnerTypeMenu(engine);
				break;
			case EditorMode::SpawnerDetails:
				renderSpawnerDetailsMenu(engine);
				break;
			case EditorMode::ChestDetails:
				renderChestDetailsMenu(engine);
				break;
			case EditorMode::NPCSelection:
				renderNPCSelectionMenu(engine);
				break;
			case EditorMode::PropDetails:
				renderPropDetailsMenu(engine);
				break;
			case EditorMode::TeleportDetails:
				renderTeleportDetailsMenu(engine);
				break;
			case EditorMode::BossTriggerDetails:
				renderBossTriggerDetailsMenu(engine);
				break;
			case EditorMode::NavBlockerDetails:
				renderNavBlockerDetailsMenu(engine);
				break;
			case EditorMode::ItemSelection:
				renderItemSelectionMenu(engine);
				break;
			case EditorMode::KeySelection:
				renderKeySelectionMenu(engine);
				break;
			case EditorMode::ConfirmOverwrite:
				renderConfirmOverwriteDialog();
				break;
			case EditorMode::None:
				break;
		}
	}

	void DevLevel::loadAvailableMapModels() {
		_map_model_options.clear();
		_map_model_options.push_back(PLACEHOLDER_MODEL);

		const std::filesystem::path maps_dir = resolveAssetPath("maps");
		if (!std::filesystem::exists(maps_dir))
			return;

		for (const auto& entry : std::filesystem::directory_iterator(maps_dir)) {
			if (!entry.is_regular_file())
				continue;

			const std::string extension = entry.path().extension().string();
			if (extension == ".glb" || extension == ".gltf")
				_map_model_options.push_back(entry.path().filename().string());
		}

		std::sort(_map_model_options.begin() + 1, _map_model_options.end());
	}

	void DevLevel::loadAvailableLocations() {
		_location_options.clear();

		const std::filesystem::path locations_dir = resolveAssetPath("data/locations");
		if (std::filesystem::exists(locations_dir)) {
			for (const auto& entry : std::filesystem::directory_iterator(locations_dir)) {
				if (!entry.is_regular_file() || entry.path().extension() != ".json")
					continue;

				const std::string filename = entry.path().filename().string();
				if (filename.rfind("objects_", 0) == 0)
					continue;

				json data;
				if (!readJsonFile(entry.path(), data))
					continue;

				LocationOption option;
				option.path = entry.path();
				option.location_name = data.value("name", displayNameFromStem(entry.path().stem().string()));
				option.display_name = option.location_name + " (" + filename + ")";

				if (data.contains("objects_file") && data["objects_file"].is_string())
					option.objects_path = locations_dir / data["objects_file"].get<std::string>();
				else
					option.objects_path = getObjectsFilePath(option.path);

				_location_options.push_back(std::move(option));
			}
		}
	}

	void DevLevel::loadAvailableBossIds() {
		_boss_id_options.clear();

		json root;
		if (readJsonFile(resolveAssetPath("data/bosses.json"), root) &&
			root.contains("bosses") &&
			root["bosses"].is_array()) {
			for (const auto& boss : root["bosses"]) {
				const std::string id = boss.value("id", "");
				if (!id.empty())
					_boss_id_options.push_back(id);
			}
		}

		if (_boss_id_options.empty())
			_boss_id_options.push_back("devil_lord");
	}

	void DevLevel::initializeNewLocation(Core::Engine* engine) {
		stopTestLevel(engine);

		_selected_location_index = static_cast<int>(_location_options.size());
		_selected_map_model_index = 0;
		_active_map_model = PLACEHOLDER_MODEL;
		_active_map_scale = 1.0f;
		_active_map_offset = {0.0f, 0.0f, 0.0f};
		_active_map_rotation = {0.0f, 0.0f, 0.0f};
		_navmesh_min_walkable_height = 0.0f;
		_camera_zoom = 0.75f;
		_player_spawn = {0.0f, 0.0f};
		_has_player_spawn = true;
		_active_location_name = "Nowa lokacja";
		_location_name_buffer = _active_location_name;
		_location_file_buffer = "nowa_lokacja.json";
		_placed_objects.clear();

		applyLocationStateToBuffers();
		if (engine)
			engine->getCamera().resetZoom(_camera_zoom);
		reloadMapFromEditor(engine, true);
		_status_message = "Pusta lokacja gotowa.";
		_has_unsaved_changes = false;
	}

	void DevLevel::loadLocationFromOption(Core::Engine* engine, const int option_index) {
		if (option_index < 0)
			return;

		if (option_index >= static_cast<int>(_location_options.size())) {
			initializeNewLocation(engine);
			return;
		}

		stopTestLevel(engine);

		const LocationOption& option = _location_options[option_index];
		_selected_location_index = option_index;
		_placed_objects.clear();
		_active_location_name = option.location_name;
		_location_name_buffer = option.location_name;
		_location_file_buffer = sanitizeJsonFilename(slugify(option.location_name), slugify(option.location_name));
		_active_map_model = PLACEHOLDER_MODEL;
		_active_map_scale = 1.0f;
		_active_map_offset = {0.0f, 0.0f, 0.0f};
		_active_map_rotation = {0.0f, 0.0f, 0.0f};
		_navmesh_min_walkable_height = 0.0f;
		_camera_zoom = 0.75f;
		_player_spawn = {0.0f, 0.0f};
		_has_player_spawn = true;

		json root;
		if (readJsonFile(option.path, root)) {
			_active_location_name = root.value("name", option.location_name);
			_location_name_buffer = _active_location_name;
			_location_file_buffer = option.path.filename().string();

			if (root.contains("map") && root["map"].is_object()) {
				const auto& map = root["map"];
				_active_map_model = map.value("model", std::string(PLACEHOLDER_MODEL));
				_active_map_scale = map.value("scale", 1.0f);
				_active_map_offset = parseVector3(map.value("offset", json::object()), _active_map_offset);
				_active_map_rotation = parseVector3(map.value("rotation", json::object()), _active_map_rotation);
			}

			if (root.contains("player_spawn") && root["player_spawn"].is_object())
				_player_spawn = parseVector2(root["player_spawn"], _player_spawn);

			_navmesh_min_walkable_height = readNavmeshMinHeight(root, _navmesh_min_walkable_height);
			_camera_zoom = readCameraZoom(root, _camera_zoom);
			loadPlacedObjectsFromFile(option.objects_path, _active_location_name);
		}

		_selected_map_model_index = 0;
		for (int index = 0; index < static_cast<int>(_map_model_options.size()); ++index) {
			if (_map_model_options[index] == _active_map_model) {
				_selected_map_model_index = index;
				break;
			}
		}

		applyLocationStateToBuffers();
		if (engine)
			engine->getCamera().resetZoom(_camera_zoom);
		reloadMapFromEditor(engine, true);
		_status_message = "Wczytano lokacje: " + _active_location_name;
		_has_unsaved_changes = false;
	}

	void DevLevel::loadPlacedObjectsFromFile(const std::filesystem::path& path, const std::string& location_filter) {
		if (path.empty() || !std::filesystem::exists(path))
			return;

		json data;
		if (!readJsonFile(path, data))
			return;

		const json* entities = nullptr;
		if (data.is_array()) {
			entities = &data;
		} else if (data.is_object() && data.contains("entities") && data["entities"].is_array()) {
			entities = &data["entities"];
		}

		if (!entities)
			return;

		for (const auto& entry : *entities) {
			if (!location_filter.empty() && entry.value("location", location_filter) != location_filter)
				continue;

			_placed_objects.push_back(parsePlacedObject(entry));
		}
	}

	DevLevel::PlacedObject DevLevel::parsePlacedObject(const json& data) const {
		PlacedObject placed_object;
		placed_object.raw_data = data.is_object() ? data : json::object();
		placed_object.type = data.value("type", "");
		placed_object.category = data.value("category", categoryFromEntityType(placed_object.type));
		placed_object.name = data.value("name", placed_object.type.empty() ? "Object" : placed_object.type);
		placed_object.position = {data.value("x", 0.0f), data.value("y", 0.0f)};
		placed_object.spawn_radius = data.value("spawn_radius", 0.0f);
		placed_object.trigger_radius = data.value("trigger_radius", 0.0f);
		placed_object.count = data.value("count", 1);
		placed_object.locked = data.value("locked", false);
		placed_object.key_id = data.value("key_id", -1);
		placed_object.blocker_width = data.value("width", 4.0f);
		placed_object.blocker_depth = data.value("depth", 4.0f);
		placed_object.blocker_height = data.value("height", 0.0f);
		placed_object.blocker_radius = data.value(
			"radius",
			std::max(placed_object.blocker_width, placed_object.blocker_depth) * 0.5f
		);
		placed_object.blocker_shape = data.value("shape", "box");

		if (data.contains("items") && data["items"].is_array()) {
			for (const auto& item_id : data["items"])
				placed_object.loot_ids.push_back(item_id.get<int>());
		}

		if (placed_object.category == "npcs") {
			placed_object.extra_value = data.value("npc_class", "");
		} else if (placed_object.category == "props") {
			if (data.contains("model") && data["model"].is_string())
				placed_object.extra_value = data["model"].get<std::string>();
			else if (data.contains("model_path") && data["model_path"].is_string())
				placed_object.extra_value = data["model_path"].get<std::string>();
			else
				placed_object.extra_value = data.value("texture", "");
		} else if (placed_object.category == "teleports") {
			placed_object.extra_value = data.value("target_location", "");
		} else if (placed_object.category == "boss_triggers") {
			placed_object.extra_value = data.value("boss_id", "");
			placed_object.spawn_radius = data.value("width", 10.0f);
			placed_object.trigger_radius = data.value("height", 4.0f);
		} else if (placed_object.category == "nav_blockers") {
			placed_object.blocker_width = data.value("width", 4.0f);
			placed_object.blocker_depth = data.value("depth", 4.0f);
			placed_object.blocker_height = data.value("height", 0.0f);
			placed_object.blocker_radius = data.value(
				"radius",
				std::max(placed_object.blocker_width, placed_object.blocker_depth) * 0.5f
			);
			placed_object.blocker_shape = data.value("shape", "box");
			if (placed_object.blocker_shape != "circle")
				placed_object.blocker_shape = "box";
		}

		return placed_object;
	}

	json DevLevel::serializePlacedObject(const PlacedObject& object) const {
		json data = object.raw_data.is_object() ? object.raw_data : json::object();
		data["x"] = object.position.x;
		data["y"] = object.position.y;
		data["name"] = object.name;
		data["type"] = object.type;
		data["category"] = object.category;

		if (object.category == "spawners") {
			data["count"] = object.count;
			data["spawn_radius"] = object.spawn_radius;
			data["trigger_radius"] = object.trigger_radius;
		} else if (object.category == "chests") {
			if (!object.loot_ids.empty())
				data["items"] = object.loot_ids;
			else
				data.erase("items");

			if (object.locked) {
				data["locked"] = true;
				data["key_id"] = object.key_id;
			} else {
				data.erase("locked");
				data.erase("key_id");
			}
		} else if (object.category == "npcs") {
			data["npc_class"] = object.extra_value;
		} else if (object.category == "props") {
			if (object.extra_value.empty())
				data.erase("model");
			else
				data["model"] = object.extra_value;
			data.erase("model_path");
			data.erase("texture");
		} else if (object.category == "teleports") {
			data["target_location"] = object.extra_value;
		} else if (object.category == "boss_triggers") {
			data["boss_id"] = object.extra_value;
			data["width"] = object.spawn_radius;
			data["height"] = object.trigger_radius;
		} else if (object.category == "nav_blockers") {
			data["shape"] = object.blocker_shape == "circle" ? "circle" : "box";
			if (object.blocker_shape == "circle") {
				data["radius"] = object.blocker_radius;
				data.erase("width");
				data.erase("depth");
			} else {
				data["width"] = object.blocker_width;
				data["depth"] = object.blocker_depth;
				data.erase("radius");
			}
			data["height"] = object.blocker_height;
			data.erase("count");
			data.erase("spawn_radius");
			data.erase("trigger_radius");
		}

		return data;
	}

	void DevLevel::applyLocationStateToBuffers() {
		_map_scale_buffer = formatFloat(_active_map_scale);
		_offset_x_buffer = formatFloat(_active_map_offset.x);
		_offset_y_buffer = formatFloat(_active_map_offset.y);
		_offset_z_buffer = formatFloat(_active_map_offset.z);
		_rotation_x_buffer = formatFloat(_active_map_rotation.x);
		_rotation_y_buffer = formatFloat(_active_map_rotation.y);
		_rotation_z_buffer = formatFloat(_active_map_rotation.z);
		_navmesh_height_buffer = formatFloat(_navmesh_min_walkable_height);
	}

	bool DevLevel::syncLocationStateFromBuffers() {
		float offset_x = 0.0f;
		float offset_y = 0.0f;
		float offset_z = 0.0f;
		float rotation_x = 0.0f;
		float rotation_y = 0.0f;
		float rotation_z = 0.0f;

		if (!tryParseFloat(_offset_x_buffer, offset_x) ||
			!tryParseFloat(_offset_y_buffer, offset_y) ||
			!tryParseFloat(_offset_z_buffer, offset_z) ||
			!tryParseFloat(_rotation_x_buffer, rotation_x) ||
			!tryParseFloat(_rotation_y_buffer, rotation_y) ||
			!tryParseFloat(_rotation_z_buffer, rotation_z)) {
			_status_message = "Bledne dane liczbowe mapy.";
			Core::Logger::errorLog("DevLevel: bledne dane liczbowe w ustawieniach mapy.");
			return false;
		}

		_active_location_name = _location_name_buffer.empty() ? "Nowa lokacja" : _location_name_buffer;
		_active_map_model =
			(_selected_map_model_index >= 0 && _selected_map_model_index < static_cast<int>(_map_model_options.size()))
				? _map_model_options[_selected_map_model_index]
				: PLACEHOLDER_MODEL;
		_active_map_scale = std::clamp(_active_map_scale, MAP_SCALE_MIN, MAP_SCALE_MAX);
		_active_map_offset = {offset_x, offset_y, offset_z};
		_active_map_rotation = {rotation_x, rotation_y, rotation_z};
		_navmesh_min_walkable_height = std::clamp(_navmesh_min_walkable_height, NAVMESH_HEIGHT_MIN, NAVMESH_HEIGHT_MAX);
		_map_scale_buffer = formatFloat(_active_map_scale);
		_navmesh_height_buffer = formatFloat(_navmesh_min_walkable_height);

		return true;
	}

	void DevLevel::reloadMapFromEditor(Core::Engine* engine, const bool move_player_to_spawn) {
		if (!engine || !_map)
			return;

		if (!syncLocationStateFromBuffers())
			return;

		stopTestLevel(engine);

		if (_active_map_model == PLACEHOLDER_MODEL) {
			_map->loadPlaceholder();
		} else {
			_map->loadMap(_active_map_model, _active_map_scale, _active_map_offset, _active_map_rotation);
		}

		applyNavMeshBlockersToMap();
		_map->setNavMeshMinWalkableHeight(_navmesh_min_walkable_height);

		if (move_player_to_spawn && _has_player_spawn) {
			if (const auto player = engine->getPlayer()) {
				Vector3 snapped_spawn = {_player_spawn.x, 0.0f, _player_spawn.y};
				if (_map->getNavMesh().isReady())
					snapped_spawn = _map->getNavMesh().getClosestWalkablePosition(snapped_spawn);

				player->setX(snapped_spawn.x);
				player->setY(snapped_spawn.z);
				player->setAltitude(snapped_spawn.y);
				player->setRespawnPoint({snapped_spawn.x, snapped_spawn.z});
				player->stop();
			}
		}

		_has_unsaved_changes = true;
		_status_message = "Mapa przeladowana.";
	}

	void DevLevel::setSpawnFromPlayer(Core::Engine* engine) {
		_player_spawn = getPlayerPosition(engine);
		_has_player_spawn = true;

		if (const auto player = engine ? engine->getPlayer() : nullptr)
			player->setRespawnPoint(_player_spawn);

		_has_unsaved_changes = true;
		_status_message = "Spawn ustawiony na pozycji gracza.";
	}

	Vector2 DevLevel::getPlayerPosition(Core::Engine* engine) const {
		if (const auto player = engine ? engine->getPlayer() : nullptr)
			return {player->getX(), player->getY()};

		return _player_spawn;
	}

	std::filesystem::path DevLevel::getLocationFilePathFromBuffer() const {
		const std::string fallback_stem = slugify(_location_name_buffer.empty() ? _active_location_name : _location_name_buffer);
		const std::string filename = sanitizeJsonFilename(_location_file_buffer, fallback_stem);
		return resolveAssetPath(std::filesystem::path("data/locations") / filename);
	}

	std::filesystem::path DevLevel::getObjectsFilePath(const std::filesystem::path& location_path) const {
		const std::filesystem::path directory = location_path.parent_path();
		const std::string stem = location_path.stem().string();
		return directory / ("objects_" + stem + ".json");
	}

	void DevLevel::requestSaveLocation(Core::Engine* engine) {
		if (engine)
			_camera_zoom = engine->getCamera().getZoomFactor();

		if (!syncLocationStateFromBuffers())
			return;

		_pending_location_save_path = getLocationFilePathFromBuffer();
		_pending_objects_save_path = getObjectsFilePath(_pending_location_save_path);

		const bool overwrites_location = std::filesystem::exists(_pending_location_save_path);
		const bool overwrites_objects = std::filesystem::exists(_pending_objects_save_path);

		if (overwrites_location || overwrites_objects) {
			_current_mode = EditorMode::ConfirmOverwrite;
			return;
		}

		saveLocationFiles(_pending_location_save_path, _pending_objects_save_path);
	}

	void DevLevel::saveLocationFiles(const std::filesystem::path& location_path, const std::filesystem::path& objects_path) {
		try {
			std::filesystem::create_directories(location_path.parent_path());
		} catch (const std::filesystem::filesystem_error& error) {
			_status_message = "Nie mozna utworzyc katalogu zapisu.";
			Core::Logger::errorLog("DevLevel: nie mozna utworzyc katalogu lokacji: " + std::string(error.what()));
			return;
		}

		json location_data;
		location_data["schema_version"] = 1;
		location_data["name"] = _active_location_name;
		location_data["map"] = {
			{"model", _active_map_model},
			{"scale", _active_map_scale},
			{"offset", vector3ToJson(_active_map_offset)},
			{"rotation", vector3ToJson(_active_map_rotation)},
		};
		location_data["navmesh"]["min_walkable_height"] = _navmesh_min_walkable_height;
		location_data["camera"]["zoom"] = _camera_zoom;
		location_data["player_spawn"] = vector2ToJson(_player_spawn);
		location_data["objects_file"] = objects_path.filename().string();

		std::ofstream location_output(location_path);
		if (!location_output.is_open()) {
			_status_message = "Nie mozna zapisac pliku lokacji.";
			Core::Logger::errorLog("DevLevel: nie mozna zapisac lokacji: " + toPathString(location_path));
			return;
		}
		location_output << location_data.dump(4);

		json objects_data;
		objects_data["schema_version"] = 1;
		objects_data["location"] = _active_location_name;
		objects_data["entities"] = json::array();

		for (const auto& object : _placed_objects) {
			json entity_data = serializePlacedObject(object);
			entity_data["location"] = _active_location_name;
			objects_data["entities"].push_back(std::move(entity_data));
		}

		std::ofstream objects_output(objects_path);
		if (!objects_output.is_open()) {
			_status_message = "Nie mozna zapisac pliku obiektow.";
			Core::Logger::errorLog("DevLevel: nie mozna zapisac obiektow: " + toPathString(objects_path));
			return;
		}
		objects_output << objects_data.dump(4);

		_has_unsaved_changes = false;
		_current_mode = EditorMode::None;
		_status_message = "Zapisano: " + location_path.filename().string() + " + " + objects_path.filename().string();

		loadAvailableLocations();
		for (int index = 0; index < static_cast<int>(_location_options.size()); ++index) {
			if (_location_options[index].path.filename() == location_path.filename()) {
				_selected_location_index = index;
				break;
			}
		}
	}

	void DevLevel::handleUIInput(Core::Engine* engine) {
		(void)engine;

		if (!IsKeyPressed(KEY_ESCAPE))
			return;

		if (_current_mode == EditorMode::ItemSelection || _current_mode == EditorMode::KeySelection) {
			_current_mode = EditorMode::ChestDetails;
		} else {
			if (_current_mode == EditorMode::NavBlockerDetails)
				applyNavMeshBlockersToMap(false);
			_current_mode = EditorMode::None;
		}

		_active_text_field = EditorTextField::None;
	}

	void DevLevel::handleEditingInput(Core::Engine* engine) {
		if (!engine)
			return;

		if (isMouseOverEditorUI())
			return;

		auto& lighting_system = engine->getLightingSystem();
		if (!lighting_system.getLights().empty()) {
			auto& primary_light = lighting_system.getLights()[PRIMARY_LIGHT_INDEX];
			bool lighting_changed = false;

			if (IsKeyDown(KEY_UP)) {
				primary_light.position.z -= LIGHT_MOVE_STEP;
				lighting_changed = true;
			}
			if (IsKeyDown(KEY_DOWN)) {
				primary_light.position.z += LIGHT_MOVE_STEP;
				lighting_changed = true;
			}
			if (IsKeyDown(KEY_LEFT)) {
				primary_light.position.x -= LIGHT_MOVE_STEP;
				lighting_changed = true;
			}
			if (IsKeyDown(KEY_RIGHT)) {
				primary_light.position.x += LIGHT_MOVE_STEP;
				lighting_changed = true;
			}
			if (IsKeyDown(KEY_PAGE_UP)) {
				primary_light.position.y += LIGHT_MOVE_STEP;
				lighting_changed = true;
			}
			if (IsKeyDown(KEY_PAGE_DOWN)) {
				primary_light.position.y -= LIGHT_MOVE_STEP;
				lighting_changed = true;
			}

			if (lighting_changed)
				lighting_system.updateLightValues(PRIMARY_LIGHT_INDEX);

			if (IsKeyPressed(KEY_S)) {
				lighting_system.saveLightingToJson(DEFAULT_LIGHTING_FILE);
				Core::Logger::debugLog("DevLevel: zapisano oswietlenie.");
			}
		}
	}

	void DevLevel::renderPlacedObjects(Core::Engine* engine) {
		if (!engine || !_map)
			return;

		const auto& camera = engine->getCamera().get();
		const auto& font = engine->getUIHandler().getFont();

		BeginMode3D(camera);

		for (const auto& object : _placed_objects) {
			const Vector3 nav_position = object.category == "nav_blockers"
				? Vector3{object.position.x, object.blocker_height, object.position.y}
				: _map->getNavMesh().getClosestWalkablePosition({object.position.x, 0.0f, object.position.y});
			const Vector3 marker_position = {object.position.x, nav_position.y + 0.5f, object.position.y};
			const Color marker_color = getCategoryColor(object.category);

			if (object.category == "nav_blockers") {
				const Vector3 blocker_center = {object.position.x, object.blocker_height, object.position.y};
				if (object.blocker_shape == "circle") {
					const float radius = std::max(0.1f, object.blocker_radius);
					DrawCylinder(blocker_center, radius, radius, 0.04f, 32, Color{50, 130, 255, 60});
					DrawCylinderWires(blocker_center, radius, radius, 0.08f, 32, Color{90, 180, 255, 220});
				} else {
					DrawPlane(
						blocker_center,
						{std::max(0.1f, object.blocker_width), std::max(0.1f, object.blocker_depth)},
						Color{50, 130, 255, 65});
					DrawCubeWires(
						blocker_center,
						std::max(0.1f, object.blocker_width),
						0.05f,
						std::max(0.1f, object.blocker_depth),
						Color{90, 180, 255, 220});
				}
			} else {
				DrawCube(marker_position, 1.0f, 1.0f, 1.0f, marker_color);
				DrawCubeWires(marker_position, 1.1f, 1.1f, 1.1f, RAYWHITE);
			}

			if (object.category == "spawners") {
				const Vector3 ground_position = {object.position.x, nav_position.y + 0.05f, object.position.y};

				if (object.spawn_radius > 0.0f) {
					DrawCircle3D(
						ground_position,
						object.spawn_radius,
						{1.0f, 0.0f, 0.0f},
						90.0f,
						ColorAlpha(YELLOW, 0.3f)
					);
					DrawCylinderWires(ground_position, object.spawn_radius, object.spawn_radius, 0.3f, 20, YELLOW);
				}

				if (object.trigger_radius > 0.0f) {
					DrawCircle3D(
						ground_position,
						object.trigger_radius,
						{1.0f, 0.0f, 0.0f},
						90.0f,
						ColorAlpha(ORANGE, 0.2f)
					);
					DrawCylinderWires(
						ground_position,
						object.trigger_radius,
						object.trigger_radius,
						0.6f,
						20,
						ORANGE
					);
				}
			} else if (object.category == "boss_triggers") {
				const Vector3 trigger_center = {object.position.x, nav_position.y + 0.15f, object.position.y};
				DrawCubeWires(
					trigger_center,
					std::max(0.1f, object.spawn_radius),
					0.3f,
					std::max(0.1f, object.trigger_radius),
					MAGENTA
				);
			}
		}

		renderNavBlockerPreview();

		EndMode3D();

		for (const auto& object : _placed_objects) {
			const Vector3 nav_position = object.category == "nav_blockers"
				? Vector3{object.position.x, object.blocker_height, object.position.y}
				: _map->getNavMesh().getClosestWalkablePosition({object.position.x, 0.0f, object.position.y});
			const Vector2 screen_position =
				GetWorldToScreen({object.position.x, nav_position.y + 2.0f, object.position.y}, camera);
			const std::string label = "[" + object.category + "] " + object.name;
			const Vector2 text_size = MeasureTextEx(font, label.c_str(), 14, 1.0f);

			DrawRectangle(
				static_cast<int>(screen_position.x - text_size.x / 2.0f - 2.0f),
				static_cast<int>(screen_position.y - 12.0f),
				static_cast<int>(text_size.x + 4.0f),
				16,
				Fade(BLACK, 0.6f)
			);
			drawDevText(
				font,
				label.c_str(),
				screen_position.x - text_size.x / 2.0f,
				screen_position.y - 10.0f,
				14,
				RAYWHITE
			);
		}
	}

	void DevLevel::deleteNearestObject(Core::Engine* engine) {
		if (_placed_objects.empty() || !engine || !_map || engine->getUIHandler().isMouseOverUI() || isMouseOverEditorUI())
			return;

		const Ray ray = GetScreenToWorldRay(GetMousePosition(), engine->getCamera().get());
		int nearest_index = -1;
		float nearest_hit_distance = std::numeric_limits<float>::max();

		for (int index = 0; index < static_cast<int>(_placed_objects.size()); ++index) {
			const auto& object = _placed_objects[index];
			const Vector3 nav_position = object.category == "nav_blockers"
				? Vector3{object.position.x, object.blocker_height, object.position.y}
				: _map->getNavMesh().getClosestWalkablePosition({object.position.x, 0.0f, object.position.y});
			const Vector3 marker_position = object.category == "nav_blockers"
				? Vector3{object.position.x, object.blocker_height, object.position.y}
				: Vector3{object.position.x, nav_position.y + 0.5f, object.position.y};
			const bool circular_blocker = object.category == "nav_blockers" && object.blocker_shape == "circle";
			const float half_x = object.category == "nav_blockers"
				? std::max(0.6f, circular_blocker ? object.blocker_radius : object.blocker_width * 0.5f)
				: 0.6f;
			const float half_z = object.category == "nav_blockers"
				? std::max(0.6f, circular_blocker ? object.blocker_radius : object.blocker_depth * 0.5f)
				: 0.6f;
			const float half_y = object.category == "nav_blockers" ? 0.25f : 0.6f;
			const BoundingBox marker_box = {
				{marker_position.x - half_x, marker_position.y - half_y, marker_position.z - half_z},
				{marker_position.x + half_x, marker_position.y + half_y, marker_position.z + half_z},
			};

			const RayCollision hit = GetRayCollisionBox(ray, marker_box);
			if (hit.hit && hit.distance < nearest_hit_distance) {
				nearest_hit_distance = hit.distance;
				nearest_index = index;
			}
		}

		if (nearest_index == -1) {
			const RayCollision ground_hit = _map->getRayCollision(ray);
			if (ground_hit.hit) {
				const Vector2 mouse_world_position = {ground_hit.point.x, ground_hit.point.z};
				float nearest_ground_distance = 3.0f;

				for (int index = 0; index < static_cast<int>(_placed_objects.size()); ++index) {
					const float distance = Vector2Distance(mouse_world_position, _placed_objects[index].position);
					if (distance < nearest_ground_distance) {
						nearest_ground_distance = distance;
						nearest_index = index;
					}
				}
			}
		}

		if (nearest_index == -1)
			return;

		Core::Logger::debugLog("DevLevel: usunieto obiekt: " + _placed_objects[nearest_index].name);
		const bool removed_nav_blocker = _placed_objects[nearest_index].category == "nav_blockers";
		_placed_objects.erase(_placed_objects.begin() + nearest_index);
		if (removed_nav_blocker)
			applyNavMeshBlockersToMap();
		_has_unsaved_changes = true;
		_status_message = "Usunieto obiekt. Zapisz lokacje, aby utrwalic zmiany.";
	}

	void DevLevel::renderNavBlockerPreview() const {
		if (_current_mode != EditorMode::NavBlockerDetails)
			return;

		const Vector3 center = {_saved_world_position.x, _temp_nav_blocker_height, _saved_world_position.y};
		if (_temp_nav_blocker_shape == "circle") {
			const float radius = std::max(0.1f, _temp_nav_blocker_radius);
			DrawCylinder(center, radius, radius, 0.06f, 40, Color{255, 210, 40, 75});
			DrawCylinderWires(center, radius, radius, 0.16f, 40, ORANGE);
			return;
		}

		DrawPlane(
			center,
			{std::max(0.1f, _temp_nav_blocker_width), std::max(0.1f, _temp_nav_blocker_depth)},
			Color{255, 210, 40, 75}
		);
		DrawCubeWires(
			center,
			std::max(0.1f, _temp_nav_blocker_width),
			0.12f,
			std::max(0.1f, _temp_nav_blocker_depth),
			ORANGE
		);
	}

	void DevLevel::testLevel(Core::Engine* engine) {
		if (!engine || _is_testing_level)
			return;

		auto& entity_manager = engine->getEntityManager();
		entity_manager.clearNonPlayerEntities();
		_spawn_manager.reset();

		for (const auto& object : _placed_objects) {
			if (object.category == "nav_blockers")
				continue;

			json entity_data = serializePlacedObject(object);
			entity_data["location"] = _active_location_name;

			if (object.category == "spawners") {
				const int spawn_count = std::max(1, object.count);
				for (int index = 0; index < spawn_count; ++index) {
					const float angle = static_cast<float>(index) * (2.0f * PI / static_cast<float>(spawn_count));
					const float offset_x = std::cos(angle) * (object.spawn_radius * 0.5f);
					const float offset_y = std::sin(angle) * (object.spawn_radius * 0.5f);

					json spawn_data = entity_data;
					spawn_data["x"] = object.position.x + offset_x;
					spawn_data["y"] = object.position.y + offset_y;

					auto entity = EntityFactory::create(object.type, spawn_data, engine, _map.get());
					if (!entity)
						continue;

					if (_map && _map->getNavMesh().isReady()) {
						const Vector3 snapped_position =
							_map->getNavMesh().getClosestWalkablePosition({entity->getX(), 0.0f, entity->getY()});
						entity->setX(snapped_position.x);
						entity->setY(snapped_position.z);
						entity->setAltitude(snapped_position.y);
					}

					SpawnPoint spawn_point;
					spawn_point.location = _active_location_name;
					spawn_point.entity_type = object.type;
					spawn_point.entity_data = spawn_data;
					spawn_point.spawn_center = object.position;
					spawn_point.trigger_radius = object.trigger_radius;
					spawn_point.spawn_radius = object.spawn_radius;
					spawn_point.entity = entity;

					const bool should_be_active = spawn_point.trigger_radius <= 0.0f;
					entity->setDormant(!should_be_active);
					spawn_point.activated = should_be_active;

					entity_manager.addEntity(entity);
					_spawn_manager.addSpawnPoint(spawn_point);
				}
			} else {
				auto entity = EntityFactory::create(object.type, entity_data, engine, _map.get());
				if (entity) {
					if (object.category == "npcs" && _map && _map->getNavMesh().isReady()) {
						const Vector3 snapped_position =
							_map->getNavMesh().getClosestWalkablePosition({entity->getX(), 0.0f, entity->getY()});
						entity->setX(snapped_position.x);
						entity->setY(snapped_position.z);
						entity->setAltitude(snapped_position.y);
					}
					entity_manager.addEntity(entity);
				}
			}
		}

		_is_testing_level = true;
		_status_message = "Test uruchomiony.";
		Core::Logger::debugLog("DevLevel: zespawnowano testowa lokacje.");
	}

	void DevLevel::stopTestLevel(Core::Engine* engine) {
		if (!engine || !_is_testing_level)
			return;

		engine->getEntityManager().clearNonPlayerEntities();
		_spawn_manager.reset();
		_is_testing_level = false;
		_status_message = "Test zakonczony.";
	}

	void DevLevel::renderLightingOverlay(Core::Engine& engine) {
		const auto& font = engine.getUIHandler().getFont();
		const int x = OVERLAY_MARGIN;
		const int y = GetScreenHeight() - 92;

		DrawRectangle(5, y - 8, 450, 86, Fade(BLACK, 0.65f));
		DrawRectangleLines(5, y - 8, 450, 86, DARKGRAY);
		drawDevText(font, "DELETE/X: usun marker pod kursorem", x, y, 15, RAYWHITE);
		drawDevText(font, "SHIFT+WASD: szybki ruch | Strzalki/PgUp/PgDn: swiatlo | S: zapisz swiatlo", x, y + 22, 15, LIGHTGRAY);
		drawDevText(font, "Obiekty sa w pamieci do klikniecia Zapisz.", x, y + 44, 15, ORANGE);
	}

	void DevLevel::renderWaterCutoffPlane(Core::Engine& engine) const {
		if (!_map)
			return;

		BeginMode3D(engine.getCamera().get());
		const Vector3 center = {0.0f, _navmesh_min_walkable_height, 0.0f};
		const Vector2 size = {WATER_PLANE_HALF_SIZE * 2.0f, WATER_PLANE_HALF_SIZE * 2.0f};
		DrawPlane(center, size, Color{50, 160, 255, 45});
		DrawCubeWires(center, size.x, 0.02f, size.y, Color{50, 190, 255, 160});
		EndMode3D();
	}

	void DevLevel::renderEditorHud(Core::Engine* engine) {
		renderMapPanel(engine);
		renderLocationPanel(engine);
		renderObjectPanel(engine);
		renderLightingOverlay(*engine);
		renderDropdownOverlays(engine);
	}

	void DevLevel::renderMapPanel(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int x = static_cast<int>(LEFT_PANEL.x);
		const int y = static_cast<int>(LEFT_PANEL.y);

		DrawRectangleRec(LEFT_PANEL, Fade(BLACK, 0.80f));
		DrawRectangleLinesEx(LEFT_PANEL, 2, ColorAlpha(RAYWHITE, 0.35f));
		drawDevText(font, "Model mapy", x + 18.0f, y + 16.0f, 28, RAYWHITE);

		const bool was_map_dropdown_open = _map_dropdown_open;
		drawDropdown(
			font,
			{LEFT_PANEL.x + 18.0f, LEFT_PANEL.y + 56.0f, 292.0f, 38.0f},
			_map_model_options,
			_selected_map_model_index,
			_map_dropdown_open,
			10
		);
		if (_map_dropdown_open && !was_map_dropdown_open) {
			_location_dropdown_open = false;
			_active_text_field = EditorTextField::None;
		}

		drawLabel(font, "Skala", x + 18, y + 110);
		if (drawSlider(
				font,
				{LEFT_PANEL.x + 118.0f, LEFT_PANEL.y + 116.0f, 186.0f, 14.0f},
				_active_map_scale,
				MAP_SCALE_MIN,
				MAP_SCALE_MAX,
				_active_text_field,
				EditorTextField::MapScale,
				SKYBLUE
			)) {
			_map_scale_buffer = formatFloat(_active_map_scale);
			_has_unsaved_changes = true;
		}

		drawLabel(font, "Offset", x + 18, y + 154);
		if (drawTextInput(font, _offset_x_buffer, x + 118, y + 148, 58, 32, _active_text_field == EditorTextField::OffsetX))
			_active_text_field = EditorTextField::OffsetX;
		if (drawTextInput(font, _offset_y_buffer, x + 182, y + 148, 58, 32, _active_text_field == EditorTextField::OffsetY))
			_active_text_field = EditorTextField::OffsetY;
		if (drawTextInput(font, _offset_z_buffer, x + 246, y + 148, 58, 32, _active_text_field == EditorTextField::OffsetZ))
			_active_text_field = EditorTextField::OffsetZ;

		drawLabel(font, "Obrot", x + 18, y + 198);
		if (drawTextInput(font, _rotation_x_buffer, x + 118, y + 192, 58, 32, _active_text_field == EditorTextField::RotationX))
			_active_text_field = EditorTextField::RotationX;
		if (drawTextInput(font, _rotation_y_buffer, x + 182, y + 192, 58, 32, _active_text_field == EditorTextField::RotationY))
			_active_text_field = EditorTextField::RotationY;
		if (drawTextInput(font, _rotation_z_buffer, x + 246, y + 192, 58, 32, _active_text_field == EditorTextField::RotationZ))
			_active_text_field = EditorTextField::RotationZ;

		drawLabel(font, "NavMesh min Y", x + 18, y + 242);
		if (drawSlider(
				font,
				{LEFT_PANEL.x + 176.0f, LEFT_PANEL.y + 248.0f, 128.0f, 14.0f},
				_navmesh_min_walkable_height,
				NAVMESH_HEIGHT_MIN,
				NAVMESH_HEIGHT_MAX,
				_active_text_field,
				EditorTextField::NavmeshMinHeight,
				LIME
			)) {
			_navmesh_height_buffer = formatFloat(_navmesh_min_walkable_height);
			_has_unsaved_changes = true;
		}

		const std::string spawn_text = "Spawn: " + formatFloat(_player_spawn.x, 1) + ", " + formatFloat(_player_spawn.y, 1);
		drawDevText(font, spawn_text.c_str(), x + 18.0f, y + 288.0f, 16, LIGHTGRAY);
		const std::string zoom_text = "Zoom kamery: " + formatFloat(_camera_zoom, 2);
		drawDevText(font, zoom_text.c_str(), x + 18.0f, y + 306.0f, 16, LIGHTGRAY);

		if (drawButton(font, "Ustaw spawn", x + 18, y + 330, 138, 40, ORANGE)) {
			_active_text_field = EditorTextField::None;
			setSpawnFromPlayer(engine);
		}

		if (drawButton(font, "Przeladuj", x + 172, y + 330, 138, 40, DARKGREEN)) {
			_active_text_field = EditorTextField::None;
			reloadMapFromEditor(engine, false);
		}
	}

	void DevLevel::renderLocationPanel(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		Rectangle panel = CENTER_PANEL;
		panel.x = GetScreenWidth() / 2.0f - panel.width / 2.0f;

		const int x = static_cast<int>(panel.x);
		const int y = static_cast<int>(panel.y);

		DrawRectangleRec(panel, Fade(BLACK, 0.82f));
		DrawRectangleLinesEx(panel, 2, ColorAlpha(RAYWHITE, 0.35f));
		drawDevText(font, "Lokacja", panel.x + 126.0f, panel.y + 16.0f, 30, RAYWHITE);

		std::vector<std::string> labels;
		labels.reserve(_location_options.size() + 1);
		for (const auto& option : _location_options)
			labels.push_back(option.display_name);
		labels.push_back("Nowa lokacja");

		const bool was_location_dropdown_open = _location_dropdown_open;
		drawDropdown(
			font,
			{panel.x + 24.0f, panel.y + 62.0f, panel.width - 48.0f, 38.0f},
			labels,
			_selected_location_index,
			_location_dropdown_open,
			8
		);
		if (_location_dropdown_open && !was_location_dropdown_open) {
			_map_dropdown_open = false;
			_active_text_field = EditorTextField::None;
		}

		drawLabel(font, "Nazwa", x + 24, y + 118);
		if (drawTextInput(font, _location_name_buffer, x + 96, y + 112, 230, 32, _active_text_field == EditorTextField::LocationName))
			_active_text_field = EditorTextField::LocationName;

		drawLabel(font, "Plik", x + 24, y + 162);
		if (drawTextInput(font, _location_file_buffer, x + 96, y + 156, 230, 32, _active_text_field == EditorTextField::LocationFile))
			_active_text_field = EditorTextField::LocationFile;

		const std::filesystem::path location_path = getLocationFilePathFromBuffer();
		const std::filesystem::path objects_path = getObjectsFilePath(location_path);
		const std::string objects_label = "Obiekty: " + objects_path.filename().string();
		drawDevText(font, objects_label.c_str(), panel.x + 24.0f, panel.y + 204.0f, 15, LIGHTGRAY);

		if (drawButton(font, "Zapisz", x + 96, y + 226, 170, 48, GREEN)) {
			_active_text_field = EditorTextField::None;
			requestSaveLocation(engine);
		}

		const std::string status = _has_unsaved_changes ? "* " + _status_message : _status_message;
		drawDevText(font, status.c_str(), panel.x + 24.0f, panel.y + 286.0f, 14, _has_unsaved_changes ? ORANGE : LIME);
	}

	void DevLevel::renderDropdownOverlays(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();

		if (_map_dropdown_open) {
			const int selected_model = drawDropdownOptions(
				font,
				{LEFT_PANEL.x + 18.0f, LEFT_PANEL.y + 56.0f, 292.0f, 38.0f},
				_map_model_options,
				_map_dropdown_open,
				10
			);
			if (selected_model >= 0) {
				_selected_map_model_index = selected_model;
				_has_unsaved_changes = true;
				_active_text_field = EditorTextField::None;
			}
		}

		if (_location_dropdown_open) {
			Rectangle panel = CENTER_PANEL;
			panel.x = GetScreenWidth() / 2.0f - panel.width / 2.0f;

			std::vector<std::string> labels;
			labels.reserve(_location_options.size() + 1);
			for (const auto& option : _location_options)
				labels.push_back(option.display_name);
			labels.push_back("Nowa lokacja");

			const int selected_location = drawDropdownOptions(
				font,
				{panel.x + 24.0f, panel.y + 62.0f, panel.width - 48.0f, 38.0f},
				labels,
				_location_dropdown_open,
				8
			);
			if (selected_location >= 0) {
				_active_text_field = EditorTextField::None;
				loadLocationFromOption(engine, selected_location);
			}
		}
	}

	void DevLevel::renderObjectPanel(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		Rectangle panel = RIGHT_PANEL;
		panel.x = GetScreenWidth() - panel.width - 20.0f;

		const int x = static_cast<int>(panel.x);
		const int y = static_cast<int>(panel.y);

		DrawRectangleRec(panel, Fade(BLACK, 0.72f));
		DrawRectangleLinesEx(panel, 2, ColorAlpha(RAYWHITE, 0.30f));
		drawDevText(font, "Obiekty", panel.x + 82.0f, panel.y + 16.0f, 30, RAYWHITE);

		const int object_button_height = 36;
		const int object_button_step = 46;
		int button_y = y + 58;
		if (drawButton(font, "Spawner", x + 28, button_y, 224, object_button_height, DARKGREEN)) {
			prepareObjectPlacementAtPlayer(engine);
			_current_mode = EditorMode::SpawnerType;
		}

		button_y += object_button_step;
		if (drawButton(font, "Skrzynia", x + 28, button_y, 224, object_button_height, DARKGREEN)) {
			prepareObjectPlacementAtPlayer(engine);
			_temp_entity_type = "chest";
			_temp_name = "Skrzynia";
			_current_mode = EditorMode::ChestDetails;
		}

		button_y += object_button_step;
		if (drawButton(font, "NPC", x + 28, button_y, 224, object_button_height, DARKGREEN)) {
			prepareObjectPlacementAtPlayer(engine);
			_current_mode = EditorMode::NPCSelection;
		}

		button_y += object_button_step;
		if (drawButton(font, "Prop", x + 28, button_y, 224, object_button_height, DARKGREEN)) {
			prepareObjectPlacementAtPlayer(engine);
			_temp_entity_type = "static_object";
			_temp_name = "Prop";
			_current_mode = EditorMode::PropDetails;
		}

		button_y += object_button_step;
		if (drawButton(font, "Gzibek prop", x + 28, button_y, 224, object_button_height, DARKGREEN)) {
			prepareObjectPlacementAtPlayer(engine);
			_temp_entity_type = "mini_mushroom_prop";
			_temp_name = "Gzibek";
			saveObject("props");
			_current_mode = EditorMode::None;
		}

		button_y += object_button_step;
		if (drawButton(font, "Teleport", x + 28, button_y, 224, object_button_height, DARKGREEN)) {
			prepareObjectPlacementAtPlayer(engine);
			_temp_entity_type = "teleport";
			_temp_name = "Teleport";
			_current_mode = EditorMode::TeleportDetails;
		}

		button_y += object_button_step;
		if (drawButton(font, "Checkpoint", x + 28, button_y, 224, object_button_height, DARKGREEN)) {
			prepareObjectPlacementAtPlayer(engine);
			_temp_entity_type = "checkpoint";
			_temp_name = "Punkt Kontrolny";
			saveObject("checkpoints");
			_current_mode = EditorMode::None;
		}

		button_y += object_button_step;
		if (drawButton(font, "Waypoint Gziba", x + 28, button_y, 224, object_button_height, DARKGREEN)) {
			prepareObjectPlacementAtPlayer(engine);
			_temp_entity_type = "checkpoint_mushroom_npc";
			_temp_name = "Checkpoint Gziba";
			saveObject("checkpoints");
			_current_mode = EditorMode::None;
		}

		button_y += object_button_step;
		if (drawButton(font, "Boss Trigger", x + 28, button_y, 224, object_button_height, DARKGREEN)) {
			prepareObjectPlacementAtPlayer(engine);
			_temp_entity_type = "boss_trigger";
			_temp_name = "Boss Trigger";
			_current_mode = EditorMode::BossTriggerDetails;
		}

		button_y += object_button_step;
		if (drawButton(font, "Nav Blocker", x + 28, button_y, 224, object_button_height, DARKGREEN)) {
			prepareObjectPlacementAtPlayer(engine);
			_temp_entity_type = "nav_blocker";
			_temp_name = "Nav Blocker";
			_temp_nav_blocker_shape = "box";
			_temp_nav_blocker_width = 6.0f;
			_temp_nav_blocker_depth = 6.0f;
			_temp_nav_blocker_radius = 3.0f;
			if (const auto player = engine ? engine->getPlayer() : nullptr) {
				_temp_nav_blocker_height = player->getAltitude();
				_nav_blocker_height_buffer = formatFloat(_temp_nav_blocker_height);
			}
			_nav_blocker_width_buffer = formatFloat(_temp_nav_blocker_width);
			_nav_blocker_depth_buffer = formatFloat(_temp_nav_blocker_depth);
			_current_mode = EditorMode::NavBlockerDetails;
		}

		button_y += 60;
		if (drawButton(font, _is_testing_level ? "Zakoncz test" : "Testuj", x + 28, button_y, 224, 44, ORANGE)) {
			if (_is_testing_level)
				stopTestLevel(engine);
			else
				testLevel(engine);
		}

		renderPlayerAnimationPanel(engine, x + 18, button_y + 62);
	}

	void DevLevel::renderPlayerAnimationPanel(Core::Engine* engine, const int x, const int y) {
		const auto& font = engine->getUIHandler().getFont();
		drawDevText(font, "Anim gracza", static_cast<float>(x), static_cast<float>(y), 18, YELLOW);

		if (PLAYER_ANIMATION_OPTIONS.empty())
			return;

		_selected_player_animation_index = std::clamp(
			_selected_player_animation_index,
			0,
			static_cast<int>(PLAYER_ANIMATION_OPTIONS.size()) - 1);

		const std::string label = std::to_string(_selected_player_animation_index) + ": " +
			PLAYER_ANIMATION_OPTIONS[static_cast<size_t>(_selected_player_animation_index)];
		drawDevText(font, label.c_str(), static_cast<float>(x), static_cast<float>(y + 26), 14, RAYWHITE);

		if (drawButton(font, "<", x, y + 50, 48, 32, DARKBLUE)) {
			_selected_player_animation_index =
				(_selected_player_animation_index - 1 + static_cast<int>(PLAYER_ANIMATION_OPTIONS.size())) %
				static_cast<int>(PLAYER_ANIMATION_OPTIONS.size());
			playSelectedPlayerAnimation(engine);
		}

		if (drawButton(font, "Play", x + 58, y + 50, 92, 32, BLUE))
			playSelectedPlayerAnimation(engine);

		if (drawButton(font, ">", x + 160, y + 50, 48, 32, DARKBLUE)) {
			_selected_player_animation_index =
				(_selected_player_animation_index + 1) % static_cast<int>(PLAYER_ANIMATION_OPTIONS.size());
			playSelectedPlayerAnimation(engine);
		}
	}

	void DevLevel::playSelectedPlayerAnimation(Core::Engine* engine) {
		const auto player = engine ? engine->getPlayer() : nullptr;
		if (!player || PLAYER_ANIMATION_OPTIONS.empty())
			return;

		const std::string& animation_name = PLAYER_ANIMATION_OPTIONS[static_cast<size_t>(_selected_player_animation_index)];
		const bool loop = animation_name.find("_Loop") != std::string::npos || animation_name.find("Idle") != std::string::npos;
		player->stop();
		player->setAnimationSpeed(1.0f);
		player->playAnimation(animation_name, loop, !loop, 0, true);
		_status_message = "Anim gracza: " + animation_name;
	}

	void DevLevel::renderConfirmOverwriteDialog() {
		const auto& font = GetFontDefault();
		const int width = 520;
		const int height = 210;
		const int x = GetScreenWidth() / 2 - width / 2;
		const int y = GetScreenHeight() / 2 - height / 2;

		DrawRectangle(x, y, width, height, Fade(BLACK, 0.92f));
		DrawRectangleLinesEx({static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)}, 2, ORANGE);

		drawDevText(font, "Nadpisac pliki?", x + 34.0f, y + 28.0f, 28, ORANGE);
		drawDevText(font, _pending_location_save_path.filename().string().c_str(), x + 34.0f, y + 76.0f, 20, RAYWHITE);
		drawDevText(font, _pending_objects_save_path.filename().string().c_str(), x + 34.0f, y + 104.0f, 20, RAYWHITE);

		if (drawButton(font, "Tak, nadpisz", x + 54, y + 150, 190, 40, MAROON))
			saveLocationFiles(_pending_location_save_path, _pending_objects_save_path);

		if (drawButton(font, "Anuluj", x + 276, y + 150, 190, 40, DARKGRAY)) {
			_current_mode = EditorMode::None;
			_status_message = "Zapis anulowany.";
		}
	}

	void DevLevel::renderSpawnerTypeMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 150;
		const int start_y = GetScreenHeight() / 2 - 150;

		drawDevText(font, "Typ przeciwnika:", start_x, start_y - 40, 24, YELLOW);

		if (drawButton(font, "Devil", start_x, start_y, 300, 40, BLUE)) {
			_temp_entity_type = "devil";
			_temp_name = "Devil";
			_current_mode = EditorMode::SpawnerDetails;
		}
		if (drawButton(font, "Bandit", start_x, start_y + 50, 300, 40, BLUE)) {
			_temp_entity_type = "bandit";
			_temp_name = "Bandit";
			_current_mode = EditorMode::SpawnerDetails;
		}
		if (drawButton(font, "Walking Dead", start_x, start_y + 100, 300, 40, BLUE)) {
			_temp_entity_type = "walking_dead";
			_temp_name = "Walking Dead";
			_current_mode = EditorMode::SpawnerDetails;
		}
		if (drawButton(font, "Ropuch (Frog)", start_x, start_y + 150, 300, 40, BLUE)) {
			_temp_entity_type = "frog";
			_temp_name = "Ropuch";
			_current_mode = EditorMode::SpawnerDetails;
		}
		if (drawButton(font, "Zly Gzibek", start_x, start_y + 200, 300, 40, BLUE)) {
			_temp_entity_type = "mini_mushroom_infected";
			_temp_name = "Zly Gzibek";
			_current_mode = EditorMode::SpawnerDetails;
		}
		if (drawButton(font, "Robal (Worm)", start_x, start_y + 250, 300, 40, BLUE)) {
			_temp_entity_type = "worm";
			_temp_name = "Robal";
			_current_mode = EditorMode::SpawnerDetails;
		}
		if (drawButton(font, "WSTECZ", start_x, start_y + 330, 300, 40, GRAY)) {
			_current_mode = EditorMode::None;
		}
	}

	void DevLevel::renderSpawnerDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 220;
		const int start_y = GetScreenHeight() / 2 - 150;
		const std::string title = "Szczegoly: " + _temp_entity_type;

		drawDevText(font, title.c_str(), start_x, start_y - 40, 24, YELLOW);
		drawLabel(font, "Nazwa:", start_x, start_y);
		if (drawTextInput(font, _temp_name, start_x, start_y + 20, 420, 36, _active_text_field == EditorTextField::ObjectName))
			_active_text_field = EditorTextField::ObjectName;

		drawLabel(font, "Liczba:", start_x, start_y + 72);
		if (drawTextInput(font, _count_buffer, start_x, start_y + 94, 100, 36, _active_text_field == EditorTextField::SpawnerCount))
			_active_text_field = EditorTextField::SpawnerCount;

		drawLabel(font, "Spawn Radius:", start_x + 122, start_y + 72);
		if (drawTextInput(font, _spawn_radius_buffer, start_x + 122, start_y + 94, 130, 36, _active_text_field == EditorTextField::SpawnerRadius))
			_active_text_field = EditorTextField::SpawnerRadius;

		drawLabel(font, "Trigger Radius:", start_x + 274, start_y + 72);
		if (drawTextInput(font, _trigger_radius_buffer, start_x + 274, start_y + 94, 146, 36, _active_text_field == EditorTextField::TriggerRadius))
			_active_text_field = EditorTextField::TriggerRadius;

		if (drawButton(font, "ZAPISZ SPAWNER", start_x, start_y + 154, 420, 50, GREEN)) {
			try {
				_temp_count = std::max(1, std::stoi(_count_buffer.empty() ? "1" : _count_buffer));
				_temp_spawn_radius = std::max(0.0f, std::stof(_spawn_radius_buffer.empty() ? "0" : _spawn_radius_buffer));
				_temp_trigger_radius = std::max(0.0f, std::stof(_trigger_radius_buffer.empty() ? "0" : _trigger_radius_buffer));
				saveObject("spawners");
				_active_text_field = EditorTextField::None;
				_current_mode = EditorMode::None;
			} catch (const std::exception& error) {
				Core::Logger::errorLog("DevLevel: bledne dane numeryczne w spawnerze: " + std::string(error.what()));
			}
		}
		if (drawButton(font, "WSTECZ", start_x, start_y + 214, 420, 40, GRAY)) {
			_current_mode = EditorMode::SpawnerType;
			_active_text_field = EditorTextField::None;
		}
	}

	void DevLevel::renderChestDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 200;
		const int start_y = GetScreenHeight() / 2 - 235;

		drawDevText(font, "Konfiguracja skrzyni:", start_x, start_y - 40, 24, YELLOW);
		drawLabel(font, "Nazwa:", start_x, start_y);
		if (drawTextInput(font, _temp_name, start_x, start_y + 20, 400, 40, _active_text_field == EditorTextField::ObjectName))
			_active_text_field = EditorTextField::ObjectName;

		const Color lock_color = _temp_chest_locked ? MAROON : DARKGREEN;
		if (drawButton(font, _temp_chest_locked ? "Zamknieta: TAK" : "Zamknieta: NIE", start_x, start_y + 70, 190, 38, lock_color)) {
			_temp_chest_locked = !_temp_chest_locked;
			if (!_temp_chest_locked) {
				_temp_key_id = -1;
				_key_id_buffer = "-1";
			}
			_active_text_field = EditorTextField::None;
		}

		if (_temp_chest_locked) {
			drawLabel(font, "ID klucza:", start_x + 215, start_y + 66);
			if (drawTextInput(font, _key_id_buffer, start_x + 215, start_y + 88, 185, 34, _active_text_field == EditorTextField::ChestKeyId))
				_active_text_field = EditorTextField::ChestKeyId;

			int preview_key_id = -1;
			const bool valid_key_id = tryParseInt(_key_id_buffer, preview_key_id);
			const auto key_template = valid_key_id ? engine->getItemDatabase().getItemTemplate(preview_key_id) : nullptr;
			const std::string key_text = valid_key_id
				? "Klucz: " + std::to_string(preview_key_id) + " - " + (key_template ? key_template->getName() : "nieznany item")
				: "Klucz: bledne ID";
			drawDevText(font, key_text.c_str(), start_x, start_y + 130, 18, key_template ? LIGHTGRAY : ORANGE);

			if (drawButton(font, "WYBIERZ KLUCZ Z BAZY", start_x, start_y + 156, 400, 38, BLUE)) {
				_active_text_field = EditorTextField::None;
				_current_mode = EditorMode::KeySelection;
			}
		} else {
			drawDevText(font, "Skrzynia bez klucza.", start_x, start_y + 130, 18, LIGHTGRAY);
		}

		std::string loot_text = "Loot IDs: ";
		for (const int item_id : _temp_loot_ids)
			loot_text += std::to_string(item_id) + ", ";
		drawDevText(font, loot_text.c_str(), start_x, start_y + 208, 18, LIGHTGRAY);

		if (drawButton(font, "+ DODAJ PRZEDMIOT", start_x, start_y + 238, 400, 40, BLUE)) {
			_active_text_field = EditorTextField::None;
			_current_mode = EditorMode::ItemSelection;
		}
		if (drawButton(font, "ZAPISZ SKRZYNIE", start_x, start_y + 298, 400, 50, GREEN)) {
			if (_temp_chest_locked) {
				int key_id = -1;
				if (!tryParseInt(_key_id_buffer, key_id) || !engine->getItemDatabase().getItemTemplate(key_id)) {
					_status_message = "Podaj poprawne ID klucza z bazy itemow.";
					Core::Logger::errorLog("DevLevel: bledny key_id skrzyni: " + _key_id_buffer);
					return;
				}
				_temp_key_id = key_id;
			} else {
				_temp_key_id = -1;
			}

			saveObject("chests");
			_active_text_field = EditorTextField::None;
			_current_mode = EditorMode::None;
		}
		if (drawButton(font, "WSTECZ", start_x, start_y + 358, 400, 40, GRAY)) {
			_active_text_field = EditorTextField::None;
			_current_mode = EditorMode::None;
		}
	}

	void DevLevel::renderNPCSelectionMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 150;
		const int start_y = GetScreenHeight() / 2 - 100;

		drawDevText(font, "Wybierz NPC:", start_x, start_y - 40, 24, YELLOW);
		if (drawButton(font, "KOT (Cat)", start_x, start_y, 300, 50, BLUE)) {
			_temp_entity_type = "npc";
			_temp_name = "Kot";
			_temp_extra_value = "cat";
			saveObject("npcs");
			_current_mode = EditorMode::None;
		}
		if (drawButton(font, "Gzib (Mushroom)", start_x, start_y + 60, 300, 50, BLUE)) {
			_temp_entity_type = "npc";
			_temp_name = "Gzib";
			_temp_extra_value = "mushroom";
			saveObject("npcs");
			_current_mode = EditorMode::None;
		}
		if (drawButton(font, "Soltys", start_x, start_y + 120, 300, 50, BLUE)) {
			_temp_entity_type = "npc";
			_temp_name = "Soltys";
			_temp_extra_value = "village_head";
			saveObject("npcs");
			_current_mode = EditorMode::None;
		}
		if (drawButton(font, "Szeptucha", start_x, start_y + 180, 300, 50, BLUE)) {
			_temp_entity_type = "npc";
			_temp_name = "Szeptucha";
			_temp_extra_value = "szeptucha";
			saveObject("npcs");
			_current_mode = EditorMode::None;
		}
		if (drawButton(font, "Zwloki Wandy", start_x, start_y + 240, 300, 50, BLUE)) {
			_temp_entity_type = "npc";
			_temp_name = "Zwloki Wandy";
			_temp_extra_value = "wanda_corpse";
			saveObject("npcs");
			_current_mode = EditorMode::None;
		}
		if (drawButton(font, "WSTECZ", start_x, start_y + 310, 300, 50, GRAY)) {
			_current_mode = EditorMode::None;
		}
	}

	void DevLevel::renderPropDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 200;
		const int start_y = GetScreenHeight() / 2 - 150;

		drawDevText(font, "Konfiguracja propa:", start_x, start_y - 40, 24, YELLOW);
		drawLabel(font, "Nazwa:", start_x, start_y);
		if (drawTextInput(font, _temp_name, start_x, start_y + 20, 400, 40, _active_text_field == EditorTextField::ObjectName))
			_active_text_field = EditorTextField::ObjectName;

		drawLabel(font, "Model:", start_x, start_y + 70);
		if (drawTextInput(font, _prop_model_path_buffer, start_x, start_y + 90, 400, 40, _active_text_field == EditorTextField::PropModel))
			_active_text_field = EditorTextField::PropModel;

		if (drawButton(font, "ZAPISZ PROP", start_x, start_y + 150, 400, 50, GREEN)) {
			_temp_extra_value = _prop_model_path_buffer;
			saveObject("props");
			_active_text_field = EditorTextField::None;
			_current_mode = EditorMode::None;
		}
		if (drawButton(font, "WSTECZ", start_x, start_y + 210, 400, 40, GRAY)) {
			_active_text_field = EditorTextField::None;
			_current_mode = EditorMode::None;
		}
	}

	void DevLevel::renderTeleportDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 200;
		const int start_y = GetScreenHeight() / 2 - 150;

		std::vector<std::string> target_locations;
		target_locations.reserve(_location_options.size() + 1);
		for (const auto& option : _location_options)
			target_locations.push_back(option.location_name);
		if (std::find(target_locations.begin(), target_locations.end(), _active_location_name) == target_locations.end())
			target_locations.push_back(_active_location_name);
		if (target_locations.empty())
			target_locations.push_back("Nowa lokacja");
		_selected_teleport_target_index = std::clamp(
			_selected_teleport_target_index,
			0,
			static_cast<int>(target_locations.size()) - 1
		);

		drawDevText(font, "Konfiguracja teleportu:", start_x, start_y - 40, 24, YELLOW);
		drawLabel(font, "Nazwa:", start_x, start_y);
		if (drawTextInput(font, _temp_name, start_x, start_y + 20, 400, 40, _active_text_field == EditorTextField::ObjectName))
			_active_text_field = EditorTextField::ObjectName;

		drawLabel(font, "Target Location:", start_x, start_y + 70);
		const bool was_target_open = _teleport_target_dropdown_open;
		drawDropdown(
			font,
			{static_cast<float>(start_x), static_cast<float>(start_y + 92), 400.0f, 40.0f},
			target_locations,
			_selected_teleport_target_index,
			_teleport_target_dropdown_open,
			8
		);
		if (_teleport_target_dropdown_open && !was_target_open) {
			_boss_dropdown_open = false;
			_active_text_field = EditorTextField::None;
		}

		if (!_teleport_target_dropdown_open && drawButton(font, "ZAPISZ TELEPORT", start_x, start_y + 150, 400, 50, GREEN)) {
			_temp_extra_value = target_locations[_selected_teleport_target_index];
			saveObject("teleports");
			_active_text_field = EditorTextField::None;
			_current_mode = EditorMode::None;
		}
		if (!_teleport_target_dropdown_open && drawButton(font, "WSTECZ", start_x, start_y + 210, 400, 40, GRAY)) {
			_active_text_field = EditorTextField::None;
			_current_mode = EditorMode::None;
		}

		if (_teleport_target_dropdown_open) {
			const int selected_target = drawDropdownOptions(
				font,
				{static_cast<float>(start_x), static_cast<float>(start_y + 92), 400.0f, 40.0f},
				target_locations,
				_teleport_target_dropdown_open,
				8
			);
			if (selected_target >= 0)
				_selected_teleport_target_index = selected_target;
		}
	}

	void DevLevel::renderBossTriggerDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 200;
		const int start_y = GetScreenHeight() / 2 - 165;

		_selected_boss_index = std::clamp(_selected_boss_index, 0, static_cast<int>(_boss_id_options.size()) - 1);

		drawDevText(font, "Konfiguracja Boss Triggera:", start_x, start_y - 40, 24, YELLOW);
		drawLabel(font, "Nazwa:", start_x, start_y);
		if (drawTextInput(font, _temp_name, start_x, start_y + 20, 400, 40, _active_text_field == EditorTextField::ObjectName))
			_active_text_field = EditorTextField::ObjectName;

		drawLabel(font, "Boss:", start_x, start_y + 70);
		const bool was_boss_open = _boss_dropdown_open;
		drawDropdown(
			font,
			{static_cast<float>(start_x), static_cast<float>(start_y + 92), 400.0f, 40.0f},
			_boss_id_options,
			_selected_boss_index,
			_boss_dropdown_open,
			8
		);
		if (_boss_dropdown_open && !was_boss_open) {
			_teleport_target_dropdown_open = false;
			_active_text_field = EditorTextField::None;
		}

		drawLabel(font, "Szerokosc:", start_x, start_y + 142);
		if (drawTextInput(font, _boss_width_buffer, start_x, start_y + 164, 180, 38, _active_text_field == EditorTextField::BossTriggerWidth))
			_active_text_field = EditorTextField::BossTriggerWidth;

		drawLabel(font, "Wysokosc:", start_x + 220, start_y + 142);
		if (drawTextInput(font, _boss_height_buffer, start_x + 220, start_y + 164, 180, 38, _active_text_field == EditorTextField::BossTriggerHeight))
			_active_text_field = EditorTextField::BossTriggerHeight;

		if (!_boss_dropdown_open && drawButton(font, "ZAPISZ BOSS TRIGGER", start_x, start_y + 224, 400, 50, GREEN)) {
			try {
				_temp_extra_value = _boss_id_options[_selected_boss_index];
				_temp_spawn_radius = std::max(0.1f, std::stof(_boss_width_buffer.empty() ? "10.0" : _boss_width_buffer));
				_temp_trigger_radius = std::max(0.1f, std::stof(_boss_height_buffer.empty() ? "4.0" : _boss_height_buffer));
				saveObject("boss_triggers");
				_active_text_field = EditorTextField::None;
				_current_mode = EditorMode::None;
			} catch (const std::exception& error) {
				Core::Logger::errorLog("DevLevel: bledne dane boss triggera: " + std::string(error.what()));
			}
		}
		if (!_boss_dropdown_open && drawButton(font, "WSTECZ", start_x, start_y + 284, 400, 40, GRAY)) {
			_active_text_field = EditorTextField::None;
			_current_mode = EditorMode::None;
		}

		if (_boss_dropdown_open) {
			const int selected_boss = drawDropdownOptions(
				font,
				{static_cast<float>(start_x), static_cast<float>(start_y + 92), 400.0f, 40.0f},
				_boss_id_options,
				_boss_dropdown_open,
				8
			);
			if (selected_boss >= 0)
				_selected_boss_index = selected_boss;
		}
	}

	void DevLevel::renderNavBlockerDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 250;
		const int start_y = GetScreenHeight() / 2 - 260;

		drawDevText(font, "Bloker NavMesha:", start_x, start_y - 40, 24, YELLOW);
		drawLabel(font, "Nazwa:", start_x, start_y);
		if (drawTextInput(font, _temp_name, start_x, start_y + 20, 500, 38, _active_text_field == EditorTextField::ObjectName))
			_active_text_field = EditorTextField::ObjectName;

		bool changed = false;
		const bool is_circle = _temp_nav_blocker_shape == "circle";
		if (drawButton(font, is_circle ? "Ksztalt: Okrag" : "Ksztalt: Prostokat", start_x, start_y + 72, 240, 40, is_circle ? PURPLE : DARKBLUE)) {
			_temp_nav_blocker_shape = is_circle ? "box" : "circle";
			changed = true;
			_active_text_field = EditorTextField::None;
		}

		drawDevText(font, "Podglad zolty: dopiero ZAPISZ BLOKER utrwala go w lokacji.", start_x, start_y + 122, 17, LIGHTGRAY);

		int row_y = start_y + 165;
		if (_temp_nav_blocker_shape == "circle") {
			drawLabel(font, "Promien:", start_x, row_y);
			changed |= drawSlider(
				font,
				{static_cast<float>(start_x), static_cast<float>(row_y + 24), 500.0f, 12.0f},
				_temp_nav_blocker_radius,
				NAV_BLOCKER_RADIUS_MIN,
				NAV_BLOCKER_RADIUS_MAX,
				_active_text_field,
				EditorTextField::NavBlockerRadius,
				ORANGE
			);
			row_y += 70;
		} else {
			drawLabel(font, "Szerokosc X:", start_x, row_y);
			changed |= drawSlider(
				font,
				{static_cast<float>(start_x), static_cast<float>(row_y + 24), 500.0f, 12.0f},
				_temp_nav_blocker_width,
				NAV_BLOCKER_SIZE_MIN,
				NAV_BLOCKER_SIZE_MAX,
				_active_text_field,
				EditorTextField::NavBlockerWidth,
				SKYBLUE
			);

			row_y += 70;
			drawLabel(font, "Glebokosc Z:", start_x, row_y);
			changed |= drawSlider(
				font,
				{static_cast<float>(start_x), static_cast<float>(row_y + 24), 500.0f, 12.0f},
				_temp_nav_blocker_depth,
				NAV_BLOCKER_SIZE_MIN,
				NAV_BLOCKER_SIZE_MAX,
				_active_text_field,
				EditorTextField::NavBlockerDepth,
				SKYBLUE
			);
			row_y += 70;
		}

		drawLabel(font, "Wysokosc Y:", start_x, row_y);
		changed |= drawSlider(
			font,
			{static_cast<float>(start_x), static_cast<float>(row_y + 24), 500.0f, 12.0f},
			_temp_nav_blocker_height,
			NAV_BLOCKER_HEIGHT_MIN,
			NAV_BLOCKER_HEIGHT_MAX,
			_active_text_field,
			EditorTextField::NavBlockerHeight,
			GOLD
		);

		if (changed) {
			_nav_blocker_width_buffer = formatFloat(_temp_nav_blocker_width);
			_nav_blocker_depth_buffer = formatFloat(_temp_nav_blocker_depth);
			_nav_blocker_height_buffer = formatFloat(_temp_nav_blocker_height);
		}

		const int button_y = row_y + 72;
		if (drawButton(font, "ZAPISZ BLOKER", start_x, button_y, 500, 50, GREEN)) {
			saveObject("nav_blockers");
			_active_text_field = EditorTextField::None;
			_current_mode = EditorMode::None;
		}

		if (drawButton(font, "WSTECZ", start_x, button_y + 62, 500, 40, GRAY)) {
			applyNavMeshBlockersToMap(false);
			_active_text_field = EditorTextField::None;
			_current_mode = EditorMode::None;
		}
	}

	void DevLevel::renderItemSelectionMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 250;
		const int start_y = GetScreenHeight() / 2 - 250;

		drawDevText(font, "Wybierz przedmiot do dodania:", start_x, start_y - 40, 24, YELLOW);

		int row = 0;
		for (const auto& [item_id, item] : engine->getItemDatabase().getAllTemplates()) {
			if (row >= MAX_VISIBLE_ITEMS)
				break;

			const std::string button_text = std::to_string(item_id) + ": " + item->getName();
			if (drawButton(font, button_text.c_str(), start_x, start_y + row * 45, 500, 40, DARKBLUE)) {
				_temp_loot_ids.push_back(item_id);
				_current_mode = EditorMode::ChestDetails;
			}
			++row;
		}

		if (drawButton(font, "WSTECZ", start_x, start_y + 500, 500, 40, GRAY))
			_current_mode = EditorMode::ChestDetails;
	}

	void DevLevel::renderKeySelectionMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 250;
		const int start_y = GetScreenHeight() / 2 - 250;

		drawDevText(font, "Wybierz klucz do skrzyni:", start_x, start_y - 40, 24, YELLOW);

		int row = 0;
		for (const auto& [item_id, item] : engine->getItemDatabase().getAllTemplates()) {
			if (row >= MAX_VISIBLE_ITEMS)
				break;

			const std::string button_text = std::to_string(item_id) + ": " + item->getName();
			if (drawButton(font, button_text.c_str(), start_x, start_y + row * 45, 500, 40, DARKBLUE)) {
				_temp_key_id = item_id;
				_key_id_buffer = std::to_string(item_id);
				_current_mode = EditorMode::ChestDetails;
			}
			++row;
		}

		if (drawButton(font, "WSTECZ", start_x, start_y + 500, 500, 40, GRAY))
			_current_mode = EditorMode::ChestDetails;
	}

	void DevLevel::resetEditorState() {
		_temp_entity_type.clear();
		_temp_name.clear();
		_temp_count = 1;
		_temp_spawn_radius = 5.0f;
		_temp_trigger_radius = 15.0f;
		_temp_loot_ids.clear();
		_temp_chest_locked = false;
		_temp_key_id = -1;
		_temp_extra_value.clear();

		_count_buffer = "1";
		_spawn_radius_buffer = "5.0";
		_trigger_radius_buffer = "15.0";
		_prop_model_path_buffer.clear();
		_boss_width_buffer = "10.0";
		_boss_height_buffer = "4.0";
		_nav_blocker_width_buffer = "4.0";
		_nav_blocker_depth_buffer = "4.0";
		_nav_blocker_height_buffer = "0.0";
		_key_id_buffer = "-1";
		_temp_nav_blocker_width = 4.0f;
		_temp_nav_blocker_depth = 4.0f;
		_temp_nav_blocker_height = 0.0f;
		_temp_nav_blocker_radius = 2.0f;
		_temp_nav_blocker_shape = "box";
		_selected_teleport_target_index = 0;
		_selected_boss_index = 0;
		_teleport_target_dropdown_open = false;
		_boss_dropdown_open = false;
		_active_text_field = EditorTextField::None;
	}

	void DevLevel::prepareObjectPlacementAtPlayer(Core::Engine* engine) {
		stopTestLevel(engine);
		resetEditorState();
		_saved_world_position = getPlayerPosition(engine);
	}

	void DevLevel::saveObject(const std::string& category) {
		PlacedObject placed_object;
		placed_object.category = category;
		placed_object.name = _temp_name.empty() ? (_temp_entity_type.empty() ? category : _temp_entity_type) : _temp_name;
		placed_object.type = _temp_entity_type;
		placed_object.position = _saved_world_position;
		placed_object.spawn_radius = _temp_spawn_radius;
		placed_object.trigger_radius = _temp_trigger_radius;
		placed_object.count = _temp_count;
		placed_object.loot_ids = _temp_loot_ids;
		placed_object.locked = _temp_chest_locked;
		placed_object.key_id = _temp_key_id;
		placed_object.blocker_width = _temp_nav_blocker_width;
		placed_object.blocker_depth = _temp_nav_blocker_depth;
		placed_object.blocker_height = _temp_nav_blocker_height;
		placed_object.blocker_radius = _temp_nav_blocker_radius;
		placed_object.blocker_shape = _temp_nav_blocker_shape == "circle" ? "circle" : "box";
		placed_object.extra_value = _temp_extra_value;
		placed_object.raw_data = json::object();

		_placed_objects.push_back(std::move(placed_object));
		if (category == "nav_blockers")
			applyNavMeshBlockersToMap(false);
		_has_unsaved_changes = true;
		_status_message = "Dodano obiekt. Kliknij Zapisz, aby zapisac pliki.";
	}

	std::vector<NavMeshBlocker> DevLevel::collectNavMeshBlockers(const bool include_preview) const {
		std::vector<NavMeshBlocker> blockers;
		for (const auto& object : _placed_objects) {
			if (object.category != "nav_blockers")
				continue;

			blockers.push_back({
				object.position,
				std::max(0.1f, object.blocker_width),
				std::max(0.1f, object.blocker_depth),
				object.blocker_height,
				navBlockerShapeFromString(object.blocker_shape),
				std::max(0.1f, object.blocker_radius)
			});
		}

		if (include_preview && _current_mode == EditorMode::NavBlockerDetails) {
			blockers.push_back({
				_saved_world_position,
				std::max(0.1f, _temp_nav_blocker_width),
				std::max(0.1f, _temp_nav_blocker_depth),
				_temp_nav_blocker_height,
				navBlockerShapeFromString(_temp_nav_blocker_shape),
				std::max(0.1f, _temp_nav_blocker_radius)
			});
		}
		return blockers;
	}

	void DevLevel::applyNavMeshBlockersToMap(const bool include_preview) {
		if (_map)
			_map->setNavMeshBlockers(collectNavMeshBlockers(include_preview));
	}

	bool DevLevel::isMouseOverEditorUI() const {
		const Vector2 mouse = GetMousePosition();
		Rectangle center_panel = CENTER_PANEL;
		center_panel.x = GetScreenWidth() / 2.0f - center_panel.width / 2.0f;
		Rectangle right_panel = RIGHT_PANEL;
		right_panel.x = GetScreenWidth() - right_panel.width - 20.0f;

		if (CheckCollisionPointRec(mouse, LEFT_PANEL) ||
			CheckCollisionPointRec(mouse, center_panel) ||
			CheckCollisionPointRec(mouse, right_panel)) {
			return true;
		}

		if (_current_mode != EditorMode::None)
			return true;

		return false;
	}

} // namespace Nawia::World
