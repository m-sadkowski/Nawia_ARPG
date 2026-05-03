#include "DevLevel.h"

#include <Map.h>
#include <Engine.h>
#include <Logger.h>
#include <MathUtils.h>

#include <fstream>
#include <filesystem>
#include <json.hpp>

using json = nlohmann::json;

namespace Nawia::World {

	void DevLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu DevLevel...");

		_map = std::make_unique<Core::Map>(engine->getResourceManager());
		_map->loadMap("forest.glb", 2.f, { 0.0f, 0.0f, 0.0f }, { 0, 0, 0 });
		
		engine->getLightingSystem().loadLightingFromJson("../assets/maps/forest_lighting.json");

		auto& em = engine->getEntityManager();
		em.clearNonPlayerEntities();
	}

	void DevLevel::handleInput(Core::Engine* engine) {
		if (_is_typing) {
			// Handle keyboard typing
			int key = GetCharPressed();
			while (key > 0) {
				if ((key >= 32) && (key <= 125)) {
					_input_text += (char)key;
				}
				key = GetCharPressed();
			}

			if (IsKeyPressed(KEY_BACKSPACE))
				if (!_input_text.empty()) 
					_input_text.pop_back();

			if (IsKeyPressed(KEY_ENTER)) {
				if (!_input_text.empty()) {
					saveObjectToJson(_input_text, _saved_iso_pos.x, _saved_iso_pos.y);
					Core::Logger::debugLog("Zapisano prop: " + _input_text);
				}
				_is_typing = false;
				_input_text = "";
			}
			
			if (IsKeyPressed(KEY_ESCAPE)) {
				_is_typing = false;
				_input_text = "";
			}
		} 
		else {
			// Lighting controls
			auto& lighting = engine->getLightingSystem();
			if (!lighting.getLights().empty()) {
				auto& light = lighting.getLights()[0]; // directional light
				bool changed = false;
				if (IsKeyDown(KEY_UP)) { light.position.z -= 1.0f; changed = true; }
				if (IsKeyDown(KEY_DOWN)) { light.position.z += 1.0f; changed = true; }
				if (IsKeyDown(KEY_LEFT)) { light.position.x -= 1.0f; changed = true; }
				if (IsKeyDown(KEY_RIGHT)) { light.position.x += 1.0f; changed = true; }
				if (IsKeyDown(KEY_PAGE_UP)) { light.position.y += 1.0f; changed = true; }
				if (IsKeyDown(KEY_PAGE_DOWN)) { light.position.y -= 1.0f; changed = true; }
				
				if (changed) {
					lighting.updateLightValues(0);
				}

				if (IsKeyPressed(KEY_S)) {
					lighting.saveLightingToJson("../assets/maps/forest_lighting.json");
					Core::Logger::debugLog("Zapisano ustawienia oswietlenia");
				}
			}

			if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
				Vector2 mouse_pos = GetMousePosition();
				
				// Transform screen pos to world using ray-cast
				_saved_iso_pos = Core::screenToWorld(engine->getCamera().get(), mouse_pos.x, mouse_pos.y);
				
				_is_typing = true;
				_input_text = "";
			}
		}
	}

	void DevLevel::update(Core::Engine* engine, float dt) {}

	void DevLevel::renderUI(Core::Engine* engine) {
		auto& lighting = engine->getLightingSystem();
		if (!lighting.getLights().empty()) {
			auto& light = lighting.getLights()[0];
			std::string light_info = "Light Pos: (" + std::to_string(light.position.x) + ", " + 
									 std::to_string(light.position.y) + ", " + std::to_string(light.position.z) + ")";
			DrawText(light_info.c_str(), 10, 50, 20, RAYWHITE);
			DrawText("Use Arrows + PgUp/PgDown to adjust lighting", 10, 80, 20, RAYWHITE);
			DrawText("Press 'S' to save lighting settings", 10, 110, 20, GREEN);
		}

		if (_is_typing) {
			int screen_w = GetScreenWidth();
			int screen_h = GetScreenHeight();
			
			DrawRectangle(0, 0, screen_w, screen_h, Fade(BLACK, 0.7f));
			
			int box_w = 500;
			int box_h = 100;
			int box_x = screen_w/2 - box_w/2;
			int box_y = screen_h/2 - box_h/2;
			
			DrawRectangle(box_x, box_y, box_w, box_h, DARKGRAY);
			DrawRectangleLines(box_x, box_y, box_w, box_h, LIGHTGRAY);
			
			DrawText("Podaj nazwe propa (zapisze sie do static_objects_dev.json):", box_x + 10, box_y + 10, 16, RAYWHITE);
			DrawText(_input_text.c_str(), box_x + 10, box_y + 50, 30, GREEN);
			
			std::string coords_text = "X: " + std::to_string(_saved_iso_pos.x) + " Z: " + std::to_string(_saved_iso_pos.y);
			DrawText(coords_text.c_str(), box_x + 10, box_y + box_h + 10, 16, GREEN);
		}
	}

	void DevLevel::saveObjectToJson(const std::string& name, float x, float y) {
		std::string path = "../assets/data/static_objects_dev.json";
		
		if (!std::filesystem::exists("../assets/data"))
			std::filesystem::create_directories("../assets/data");
		
		json objects_data = json::array();
		
		if (std::filesystem::exists(path)) {
			std::ifstream file(path);
			if (file.is_open()) {
				try {
					file >> objects_data;
				} 
				catch(...) {
					// empty / corrupted
				}
			}
		}
		
		json obj;
		obj["name"] = name;
		obj["x"] = x;
		obj["y"] = y;
		
		objects_data.push_back(obj);
		
		std::ofstream out(path);
		if (out.is_open()) {
			out << objects_data.dump(4);
		}
	}

} // namespace Nawia::World
