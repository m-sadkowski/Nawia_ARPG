#include "DevLevel.h"

#include <Engine.h>
#include <Logger.h>
#include <Map.h>
#include <MathUtils.h>

#include <filesystem>
#include <fstream>
#include <json.hpp>

using json = nlohmann::json;

namespace Nawia::World {

	namespace {

		constexpr const char* k_dev_map_file = "forest.glb";
		constexpr float k_dev_map_scale = 2.0f;
		constexpr float k_light_move_step = 1.0f;
		constexpr int k_light_index = 0;
		constexpr int k_overlay_margin = 10;
		constexpr int k_overlay_font_size = 20;
		constexpr int k_prompt_box_width = 500;
		constexpr int k_prompt_box_height = 100;
		constexpr int k_prompt_title_font_size = 16;
		constexpr int k_prompt_input_font_size = 30;
		constexpr int k_prompt_box_padding = 10;
		constexpr float k_overlay_background_alpha = 0.7f;
		constexpr Vector2 k_dev_player_spawn = { -4.3f, 33.0f };

		std::filesystem::path resolveAssetPath(const std::filesystem::path& relative_asset_path) {
			return (std::filesystem::path("assets") / relative_asset_path).lexically_normal();
		}

		std::string toPathString(const std::filesystem::path& path) {
			return path.generic_string();
		}

	}

	void DevLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu DevLevel...");

		_map = std::make_unique<Core::Map>(engine->getResourceManager());
		_map->loadMap(k_dev_map_file, k_dev_map_scale, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });
		
		const auto lighting_file_path = resolveAssetPath("maps/forest_lighting.json");
		engine->getLightingSystem().loadLightingFromJson(toPathString(lighting_file_path));

		auto& entity_manager = engine->getEntityManager();
		entity_manager.clearNonPlayerEntities();

		if (const auto player = engine->getPlayer()) {
			const Vector3 snapped_spawn = _map->getNavMesh().getClosestWalkablePosition(
				{ k_dev_player_spawn.x, 0.0f, k_dev_player_spawn.y });
			player->setX(snapped_spawn.x);
			player->setY(snapped_spawn.z);
			player->setAltitude(snapped_spawn.y);
			player->setRespawnPoint({ snapped_spawn.x, snapped_spawn.z });
			player->stop();
		}
	}

	void DevLevel::handleInput(Core::Engine* engine) {
		if (_is_typing) {
			handleTypingInput();
			return;
		}

		handleEditingInput(engine);
	}

	void DevLevel::update(Core::Engine* engine, float dt) {}

	void DevLevel::renderUI(Core::Engine* engine) {
		if (!engine)
			return;

		renderLightingOverlay(*engine);

		if (_is_typing)
			renderPropPlacementPrompt();
	}

	void DevLevel::handleTypingInput() {
		int pressed_character = GetCharPressed();
		while (pressed_character > 0) {
			if (pressed_character >= 32 && pressed_character <= 125)
				_input_text += static_cast<char>(pressed_character);

			pressed_character = GetCharPressed();
		}

		if (IsKeyPressed(KEY_BACKSPACE) && !_input_text.empty())
			_input_text.pop_back();

		if (IsKeyPressed(KEY_ENTER)) {
			if (!_input_text.empty()) {
				saveObjectToJson(_input_text, _saved_world_position.x, _saved_world_position.y);
				Core::Logger::debugLog("Zapisano prop: " + _input_text);
			}

			clearTypingState();
			return;
		}

		if (IsKeyPressed(KEY_ESCAPE))
			clearTypingState();
	}

	void DevLevel::handleEditingInput(Core::Engine* engine) {
		if (!engine)
			return;

		auto& lighting_system = engine->getLightingSystem();
		if (!lighting_system.getLights().empty()) {
			auto& primary_light = lighting_system.getLights()[k_light_index];
			bool lighting_changed = false;

			if (IsKeyDown(KEY_UP)) {
				primary_light.position.z -= k_light_move_step;
				lighting_changed = true;
			}

			if (IsKeyDown(KEY_DOWN)) {
				primary_light.position.z += k_light_move_step;
				lighting_changed = true;
			}

			if (IsKeyDown(KEY_LEFT)) {
				primary_light.position.x -= k_light_move_step;
				lighting_changed = true;
			}

			if (IsKeyDown(KEY_RIGHT)) {
				primary_light.position.x += k_light_move_step;
				lighting_changed = true;
			}

			if (IsKeyDown(KEY_PAGE_UP)) {
				primary_light.position.y += k_light_move_step;
				lighting_changed = true;
			}

			if (IsKeyDown(KEY_PAGE_DOWN)) {
				primary_light.position.y -= k_light_move_step;
				lighting_changed = true;
			}

			if (lighting_changed)
				lighting_system.updateLightValues(k_light_index);

			if (IsKeyPressed(KEY_S)) {
				const auto lighting_file_path = resolveAssetPath("maps/forest_lighting.json");
				lighting_system.saveLightingToJson(toPathString(lighting_file_path));
				Core::Logger::debugLog("Zapisano ustawienia oswietlenia do: " + toPathString(lighting_file_path));
			}
		}

		if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
			return;

		const Vector2 mouse_position = GetMousePosition();
		_saved_world_position = Core::screenToWorld(engine->getCamera().get(), mouse_position.x, mouse_position.y);
		_is_typing = true;
		_input_text.clear();
	}

	void DevLevel::renderLightingOverlay(Core::Engine& engine) const {
		const auto& lights = engine.getLightingSystem().getLights();
		if (lights.empty())
			return;

		const auto& primary_light = lights[k_light_index];
		const std::string light_info = "Glowne swiatlo: (" +
			std::to_string(primary_light.position.x) + ", " +
			std::to_string(primary_light.position.y) + ", " +
			std::to_string(primary_light.position.z) + ")";

		DrawText(light_info.c_str(), k_overlay_margin, 50, k_overlay_font_size, RAYWHITE);
		DrawText("Strzalki przesuwaja X/Z, PgUp/PgDown przesuwa Y", k_overlay_margin, 80, k_overlay_font_size, RAYWHITE);
		DrawText("S zapisuje oswietlenie", k_overlay_margin, 110, k_overlay_font_size, GREEN);
		DrawText("Prawy klik zapisuje pozycje propa", k_overlay_margin, 140, k_overlay_font_size, RAYWHITE);
	}

	void DevLevel::renderPropPlacementPrompt() const {
		const int screen_width = GetScreenWidth();
		const int screen_height = GetScreenHeight();

		DrawRectangle(0, 0, screen_width, screen_height, Fade(BLACK, k_overlay_background_alpha));

		const int box_x = screen_width / 2 - k_prompt_box_width / 2;
		const int box_y = screen_height / 2 - k_prompt_box_height / 2;

		DrawRectangle(box_x, box_y, k_prompt_box_width, k_prompt_box_height, DARKGRAY);
		DrawRectangleLines(box_x, box_y, k_prompt_box_width, k_prompt_box_height, LIGHTGRAY);

		DrawText(
			"Podaj nazwe propa (zapis do assets/data/static_objects_dev.json):",
			box_x + k_prompt_box_padding,
			box_y + k_prompt_box_padding,
			k_prompt_title_font_size,
			RAYWHITE);
		DrawText(
			_input_text.c_str(),
			box_x + k_prompt_box_padding,
			box_y + 50,
			k_prompt_input_font_size,
			GREEN);

		const std::string coordinates_text =
			"X: " + std::to_string(_saved_world_position.x) +
			" Z: " + std::to_string(_saved_world_position.y);
		DrawText(
			coordinates_text.c_str(),
			box_x + k_prompt_box_padding,
			box_y + k_prompt_box_height + k_prompt_box_padding,
			k_prompt_title_font_size,
			GREEN);
	}

	void DevLevel::clearTypingState() {
		_is_typing = false;
		_input_text.clear();
	}

	void DevLevel::saveObjectToJson(const std::string& object_name, float world_x, float world_z) {
		const auto output_path = resolveAssetPath("data/static_objects_dev.json");
		if (!output_path.parent_path().empty())
			std::filesystem::create_directories(output_path.parent_path());

		json objects_data = json::array();
		
		if (std::filesystem::exists(output_path)) {
			std::ifstream file(output_path);
			if (file.is_open()) {
				try {
					file >> objects_data;
				} 
				catch(...) {
					// Pusty albo uszkodzony plik zaczynamy od nowej tablicy.
				}
			}
		}
		
		json object_json;
		object_json["name"] = object_name;
		object_json["x"] = world_x;
		object_json["y"] = world_z;
		
		objects_data.push_back(object_json);
		
		std::ofstream out(output_path);
		if (out.is_open()) {
			out << objects_data.dump(4);
		}
	}

} // namespace Nawia::World
