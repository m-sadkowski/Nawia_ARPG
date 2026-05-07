#include "DevLevel.h"

#include <Engine.h>
#include <Logger.h>
#include <Map.h>
#include <MathUtils.h>
#include <Player.h>
#include <ItemDatabase.h>
#include <EntityFactory.h>
#include <UIHandler.h>

#include <raymath.h>
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
		constexpr float k_overlay_background_alpha = 0.85f;
		constexpr Vector2 k_dev_player_spawn = { -4.3f, 33.0f };

		std::filesystem::path resolveAssetPath(const std::filesystem::path& relative_asset_path) {
			return (std::filesystem::path("assets") / relative_asset_path).lexically_normal();
		}

		std::string toPathString(const std::filesystem::path& path) {
			return path.generic_string();
		}

		void DrawDevText(const Font& font, const char* text, float x, float y, float size, Color color) {
			DrawTextEx(font, text, { x, y }, size, 1.0f, color);
		}

		bool DrawButton(const Font& font, const char* text, int x, int y, int width, int height, Color base_color) {
			Rectangle rect = { (float)x, (float)y, (float)width, (float)height };
			bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
			DrawRectangleRec(rect, hovered ? ColorAlpha(base_color, 0.8f) : base_color);
			DrawRectangleLinesEx(rect, 2, LIGHTGRAY);
			Vector2 text_size = MeasureTextEx(font, text, 20, 1.0f);
			DrawDevText(font, text, x + width / 2 - text_size.x / 2, y + height / 2 - text_size.y / 2, 20, RAYWHITE);
			return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
		}

		void DrawLabel(const Font& font, const char* text, int x, int y) {
			DrawDevText(font, text, x, y, 18, LIGHTGRAY);
		}

		bool DrawTextInput(const Font& font, std::string& buffer, int x, int y, int width, int height, bool active) {
			Rectangle rect = { (float)x, (float)y, (float)width, (float)height };
			DrawRectangleRec(rect, active ? DARKGRAY : BLACK);
			DrawRectangleLinesEx(rect, 2, active ? GREEN : GRAY);
			DrawDevText(font, buffer.c_str(), x + 5, y + height / 2 - 10, 20, RAYWHITE);
			if (active) {
				int key = GetCharPressed();
				while (key > 0) {
					if (key >= 32 && key <= 125) buffer += (char)key;
					key = GetCharPressed();
				}
				if (IsKeyPressed(KEY_BACKSPACE) && !buffer.empty()) buffer.pop_back();
			}
			return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rect);
		}

	}

	void DevLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu DevLevel (Kreator)...");

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

		// Wczytaj istniejace obiekty do sesji
		_placed_objects.clear();
		std::vector<std::string> categories = { "spawners", "chests", "npcs", "props", "teleports" };
		for (const auto& cat : categories) {
			const auto path = resolveAssetPath("data/dev/new_level_" + cat + ".json");
			if (std::filesystem::exists(path)) {
				std::ifstream file(path);
				json data;
				if (file.is_open()) try {
					file >> data;
					for (const auto& obj : data) {
						PlacedObject po;
						po.category = cat;
						po.name = obj.value("name", "Unknown");
						po.type = obj.value("type", "");
						po.position = { obj.value("x", 0.0f), obj.value("y", 0.0f) };
						po.spawn_radius = obj.value("spawn_radius", 0.0f);
						po.trigger_radius = obj.value("trigger_radius", 0.0f);
						po.count = obj.value("count", 1);
						if (obj.contains("items")) {
							for (const auto& item_id : obj["items"]) po.loot_ids.push_back(item_id.get<int>());
						}
						if (cat == "npcs") po.extra_string = obj.value("npc_class", "");
						else if (cat == "props") po.extra_string = obj.value("texture", "");
						else if (cat == "teleports") po.extra_string = obj.value("target_location", "");
						
						_placed_objects.push_back(po);
					}
				} catch(...) {}
			}
		}
	}

	void DevLevel::handleInput(Core::Engine* engine) {
		if (_current_mode != EditorMode::None) {
			handleUIInput(engine);
			return;
		}

		if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_X)) {
			deleteNearestObject(engine);
		}

		handleEditingInput(engine);
	}

	void DevLevel::update(Core::Engine* engine, float dt) {
		if (const auto player = engine->getPlayer()) {
			float base_speed = 5.0f;
			if (IsKeyDown(KEY_LEFT_SHIFT)) base_speed = 25.0f;
			player->setMovementSpeed(base_speed);

			if (_current_mode == EditorMode::None) {
				Vector2 move = { 0, 0 };
				if (IsKeyDown(KEY_W)) move.y -= 1;
				if (IsKeyDown(KEY_S)) move.y += 1;
				if (IsKeyDown(KEY_A)) move.x -= 1;
				if (IsKeyDown(KEY_D)) move.x += 1;

				if (move.x != 0 || move.y != 0) {
					move = Vector2Normalize(move);
					player->setX(player->getX() + move.x * base_speed * dt);
					player->setY(player->getY() + move.y * base_speed * dt);
				}
			}
		}

		Level::update(engine, dt);
	}

	void DevLevel::renderUI(Core::Engine* engine) {
		if (!engine || engine->isPaused()) return;

		renderPlacedObjects(engine);
		renderLightingOverlay(*engine);

		if (_current_mode != EditorMode::None) {
			DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, k_overlay_background_alpha));
			
			switch (_current_mode) {
				case EditorMode::MainMenu: renderMainMenu(engine); break;
				case EditorMode::SpawnerType: renderSpawnerTypeMenu(engine); break;
				case EditorMode::SpawnerDetails: renderSpawnerDetailsMenu(engine); break;
				case EditorMode::ChestDetails: renderChestDetailsMenu(engine); break;
				case EditorMode::NPCSelection: renderNPCSelectionMenu(engine); break;
				case EditorMode::PropDetails: renderPropDetailsMenu(engine); break;
				case EditorMode::TeleportDetails: renderTeleportDetailsMenu(engine); break;
				case EditorMode::ItemSelection: renderItemSelectionMenu(engine); break;
				default: break;
			}
		}
	}

	void DevLevel::handleUIInput(Core::Engine* engine) {
		if (IsKeyPressed(KEY_ESCAPE)) {
			if (_current_mode == EditorMode::MainMenu) _current_mode = EditorMode::None;
			else _current_mode = EditorMode::MainMenu;
		}
	}

	void DevLevel::handleEditingInput(Core::Engine* engine) {
		if (!engine) return;

		auto& lighting_system = engine->getLightingSystem();
		if (!lighting_system.getLights().empty()) {
			auto& primary_light = lighting_system.getLights()[k_light_index];
			bool lighting_changed = false;
			if (IsKeyDown(KEY_UP)) { primary_light.position.z -= k_light_move_step; lighting_changed = true; }
			if (IsKeyDown(KEY_DOWN)) { primary_light.position.z += k_light_move_step; lighting_changed = true; }
			if (IsKeyDown(KEY_LEFT)) { primary_light.position.x -= k_light_move_step; lighting_changed = true; }
			if (IsKeyDown(KEY_RIGHT)) { primary_light.position.x += k_light_move_step; lighting_changed = true; }
			if (IsKeyDown(KEY_PAGE_UP)) { primary_light.position.y += k_light_move_step; lighting_changed = true; }
			if (IsKeyDown(KEY_PAGE_DOWN)) { primary_light.position.y -= k_light_move_step; lighting_changed = true; }
			if (lighting_changed) lighting_system.updateLightValues(k_light_index);
			if (IsKeyPressed(KEY_S)) {
				const auto lighting_file_path = resolveAssetPath("maps/forest_lighting.json");
				lighting_system.saveLightingToJson(toPathString(lighting_file_path));
				Core::Logger::debugLog("Zapisano oswietlenie.");
			}
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
			// Nie otwieraj menu, jesli klikasz nad otwartym UI (ekwipunek itp)
			if (engine->getUIHandler().isMouseOverUI() || engine->getUIHandler().isInputBlocked()) {
				return;
			}

			const Vector2 mouse_position = GetMousePosition();
			const Ray ray = GetScreenToWorldRay(mouse_position, engine->getCamera().get());
			
			const RayCollision collision = _map->getRayCollision(ray);
			if (collision.hit) {
				_saved_world_position = { collision.point.x, collision.point.z };
			} else {
				_saved_world_position = Core::screenToWorld(engine->getCamera().get(), mouse_position.x, mouse_position.y);
			}

			_current_mode = EditorMode::MainMenu;
			resetEditorState();
		}
	}

	void DevLevel::renderPlacedObjects(Core::Engine* engine) {
		const auto& camera = engine->getCamera().get();
		const auto& font = engine->getUIHandler().getFont();
		BeginMode3D(camera);

		for (const auto& obj : _placed_objects) {
			float terrain_y = _map->getNavMesh().getClosestWalkablePosition({obj.position.x, 0.0f, obj.position.y}).y;
			Vector3 pos3D = { obj.position.x, terrain_y + 0.5f, obj.position.y };
			
			Color color = GRAY;
			if (obj.category == "spawners") color = RED;
			else if (obj.category == "chests") color = GOLD;
			else if (obj.category == "npcs") color = SKYBLUE;
			else if (obj.category == "props") color = GREEN;
			else if (obj.category == "teleports") color = PURPLE;

			DrawCube(pos3D, 1.0f, 1.0f, 1.0f, color);
			DrawCubeWires(pos3D, 1.1f, 1.1f, 1.1f, RAYWHITE);

			if (obj.category == "spawners") {
				Vector3 groundPos = { obj.position.x, terrain_y + 0.05f, obj.position.y };
				
				if (obj.spawn_radius > 0) {
					DrawCircle3D(groundPos, obj.spawn_radius, { 1.0f, 0.0f, 0.0f }, 90.0f, ColorAlpha(YELLOW, 0.3f));
					DrawCylinderWires(groundPos, obj.spawn_radius, obj.spawn_radius, 0.3f, 20, YELLOW);
				}
				
				if (obj.trigger_radius > 0) {
					DrawCircle3D(groundPos, obj.trigger_radius, { 1.0f, 0.0f, 0.0f }, 90.0f, ColorAlpha(ORANGE, 0.2f));
					DrawCylinderWires(groundPos, obj.trigger_radius, obj.trigger_radius, 0.6f, 20, ORANGE);
				}
			}
		}

		EndMode3D();

		for (const auto& obj : _placed_objects) {
			float terrain_y = _map->getNavMesh().getClosestWalkablePosition({obj.position.x, 0.0f, obj.position.y}).y;
			Vector2 screen_pos = GetWorldToScreen({ obj.position.x, terrain_y + 2.0f, obj.position.y }, camera);
			std::string label = "[" + obj.category + "] " + obj.name;
			Vector2 text_size = MeasureTextEx(font, label.c_str(), 14, 1.0f);
			DrawRectangle(screen_pos.x - text_size.x / 2 - 2, screen_pos.y - 12, text_size.x + 4, 16, Fade(BLACK, 0.6f));
			DrawDevText(font, label.c_str(), screen_pos.x - text_size.x / 2, screen_pos.y - 10, 14, RAYWHITE);
		}
	}

	void DevLevel::deleteNearestObject(Core::Engine* engine) {
		if (_placed_objects.empty() || engine->getUIHandler().isMouseOverUI()) return;

		const Ray ray = GetScreenToWorldRay(GetMousePosition(), engine->getCamera().get());
		
		int nearest_idx = -1;
		float min_hit_dist = FLT_MAX;

		// 1. Najpierw sprawdzamy bezposrednie trafienie promieniem w "klocki" obiektow
		for (int i = 0; i < (int)_placed_objects.size(); ++i) {
			const auto& obj = _placed_objects[i];
			float terrain_y = _map->getNavMesh().getClosestWalkablePosition({obj.position.x, 0.0f, obj.position.y}).y;
			Vector3 pos3D = { obj.position.x, terrain_y + 0.5f, obj.position.y };
			
			BoundingBox box = { 
				{ pos3D.x - 0.6f, pos3D.y - 0.6f, pos3D.z - 0.6f }, 
				{ pos3D.x + 0.6f, pos3D.y + 0.6f, pos3D.z + 0.6f } 
			};

			RayCollision hit = GetRayCollisionBox(ray, box);
			if (hit.hit && hit.distance < min_hit_dist) {
				min_hit_dist = hit.distance;
				nearest_idx = i;
			}
		}

		// 2. Jesli nie trafilismy w klocki, sprawdzamy pozycje na ziemi (fallback)
		if (nearest_idx == -1) {
			Vector2 mouse_world;
			const RayCollision ground_hit = _map->getRayCollision(ray);
			if (ground_hit.hit) {
				mouse_world = { ground_hit.point.x, ground_hit.point.z };
				float min_dist = 3.0f;
				for (int i = 0; i < (int)_placed_objects.size(); ++i) {
					float d = Vector2Distance(mouse_world, _placed_objects[i].position);
					if (d < min_dist) {
						min_dist = d;
						nearest_idx = i;
					}
				}
			}
		}

		if (nearest_idx != -1) {
			Core::Logger::debugLog("Usunieto obiekt: " + _placed_objects[nearest_idx].name);
			_placed_objects.erase(_placed_objects.begin() + nearest_idx);
			rewriteJsonFiles();
		}
	}

	void DevLevel::testLevel(Core::Engine* engine) {
		if (!engine) return;

		auto& entity_manager = engine->getEntityManager();
		entity_manager.clearNonPlayerEntities();
		_spawn_manager.reset();

		for (const auto& obj : _placed_objects) {
			json data;
			data["x"] = obj.position.x;
			data["y"] = obj.position.y;
			data["name"] = obj.name;
			
			if (obj.category == "spawners") {
				for (int i = 0; i < obj.count; ++i) {
					float angle = (float)i * (2.0f * PI / (float)obj.count);
					float rx = std::cos(angle) * (obj.spawn_radius * 0.5f);
					float ry = std::sin(angle) * (obj.spawn_radius * 0.5f);
					
					json spawn_data = data;
					spawn_data["x"] = obj.position.x + rx;
					spawn_data["y"] = obj.position.y + ry;
					
					auto entity = EntityFactory::create(obj.type, spawn_data, engine, _map.get());
					if (entity) {
						if (_map && _map->getNavMesh().isReady()) {
							const Vector3 snapped = _map->getNavMesh().getClosestWalkablePosition({entity->getX(), 0, entity->getY()});
							entity->setX(snapped.x); entity->setY(snapped.z); entity->setAltitude(snapped.y);
						}

						SpawnPoint sp;
						sp.location = "Dev Sandbox";
						sp.entity_type = obj.type;
						sp.entity_data = spawn_data;
						sp.spawn_center = obj.position;
						sp.trigger_radius = obj.trigger_radius;
						sp.spawn_radius = obj.spawn_radius;
						sp.entity = entity;
						
						bool should_be_active = (sp.trigger_radius <= 0.0f);
						entity->setDormant(!should_be_active);
						sp.activated = should_be_active;
						
						entity_manager.addEntity(entity);
						_spawn_manager.addSpawnPoint(sp);
					}
				}
			} else {
				if (obj.category == "chests") data["items"] = obj.loot_ids;
				else if (obj.category == "npcs") data["npc_class"] = obj.extra_string;
				else if (obj.category == "props") data["texture"] = obj.extra_string;
				else if (obj.category == "teleports") data["target_location"] = obj.extra_string;

				auto entity = EntityFactory::create(obj.type, data, engine, _map.get());
				if (entity) {
					entity_manager.addEntity(entity);
				}
			}
		}

		Core::Logger::debugLog("Zespawnowano testowy poziom (z obsluga Trigger Radius).");
	}

	void DevLevel::renderLightingOverlay(Core::Engine& engine) const {
		const auto& font = engine.getUIHandler().getFont();
		int x = k_overlay_margin;
		int y = 10;
		DrawRectangle(5, 5, 450, 240, Fade(BLACK, 0.7f));
		DrawRectangleLines(5, 5, 450, 240, DARKGRAY);

		DrawDevText(font, "INSTRUKCJA KREATORA POZIOMU", x, y, 20, YELLOW); y += 30;
		DrawDevText(font, "- Prawy Klik: Otwiera menu dodawania obiektu", x, y, 16, RAYWHITE); y += 20;
		DrawDevText(font, "- SHIFT + WASD: Szybkie latanie po mapie", x, y, 16, RAYWHITE); y += 20;
		DrawDevText(font, "- DELETE / X: Usuwa obiekt pod myszka", x, y, 16, RED); y += 20;
		DrawDevText(font, "- S: Zapisuje oswietlenie mapy", x, y, 16, RAYWHITE); y += 20;
		DrawDevText(font, "- Strzalki/PgUp/PgDn: Sterowanie swiatlem", x, y, 16, GRAY); y += 30;
		
		DrawDevText(font, "LEGENDA ZASIEGOW:", x, y, 18, YELLOW); y += 25;
		DrawCircle(x + 10, y + 8, 8, ColorAlpha(YELLOW, 0.5f)); 
		DrawDevText(font, "Spawn Range (zolty)", x + 25, y, 16, RAYWHITE); y += 20;
		DrawCircle(x + 10, y + 8, 8, ColorAlpha(ORANGE, 0.5f)); 
		DrawDevText(font, "Trigger Range (pomaranczowy)", x + 25, y, 16, RAYWHITE); y += 25;

		DrawDevText(font, "Zapis do: assets/data/dev/new_level_*.json", x, y, 14, GREEN);
	}

	void DevLevel::renderMainMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		int start_x = GetScreenWidth() / 2 - 150;
		int start_y = GetScreenHeight() / 2 - 200;
		DrawDevText(font, "Wybierz typ obiektu:", start_x, start_y - 40, 24, YELLOW);
		if (DrawButton(font, "SPAWNER (Przeciwnicy)", start_x, start_y, 300, 50, DARKBLUE)) _current_mode = EditorMode::SpawnerType;
		if (DrawButton(font, "SKRZYNIA (Loot)", start_x, start_y + 60, 300, 50, DARKBLUE)) {
			_temp_entity_type = "chest"; _temp_name = "Skrzynia"; _current_mode = EditorMode::ChestDetails;
		}
		if (DrawButton(font, "NPC (Rozmowa)", start_x, start_y + 120, 300, 50, DARKBLUE)) _current_mode = EditorMode::NPCSelection;
		if (DrawButton(font, "PROP (Static Object)", start_x, start_y + 180, 300, 50, DARKBLUE)) {
			_temp_entity_type = "static_object"; _temp_name = "Prop"; _current_mode = EditorMode::PropDetails;
		}
		if (DrawButton(font, "TELEPORT", start_x, start_y + 240, 300, 50, DARKBLUE)) {
			_temp_entity_type = "teleport"; _temp_name = "Teleport"; _current_mode = EditorMode::TeleportDetails;
		}
		
		if (DrawButton(font, "TESTUJ POZIOM (Spawn)", start_x - 320, start_y, 300, 50, ORANGE)) {
			testLevel(engine);
			_current_mode = EditorMode::None;
		}

		if (DrawButton(font, "ZAPISZ SESJE", start_x - 320, start_y + 60, 300, 50, GREEN)) {
			rewriteJsonFiles();
			Core::Logger::debugLog("Sesja zapisana manualnie.");
		}

		if (DrawButton(font, "ANULUJ", start_x, start_y + 320, 300, 50, MAROON)) _current_mode = EditorMode::None;
	}

	void DevLevel::renderSpawnerTypeMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		int start_x = GetScreenWidth() / 2 - 150;
		int start_y = GetScreenHeight() / 2 - 150;
		DrawDevText(font, "Typ przeciwnika:", start_x, start_y - 40, 24, YELLOW);
		if (DrawButton(font, "Devil", start_x, start_y, 300, 40, BLUE)) { _temp_entity_type = "devil"; _current_mode = EditorMode::SpawnerDetails; }
		if (DrawButton(font, "Bandit", start_x, start_y + 50, 300, 40, BLUE)) { _temp_entity_type = "bandit"; _current_mode = EditorMode::SpawnerDetails; }
		if (DrawButton(font, "Walking Dead", start_x, start_y + 100, 300, 40, BLUE)) { _temp_entity_type = "walking_dead"; _current_mode = EditorMode::SpawnerDetails; }
		if (DrawButton(font, "WSTECZ", start_x, start_y + 180, 300, 40, GRAY)) _current_mode = EditorMode::MainMenu;
	}

	void DevLevel::renderSpawnerDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		int start_x = GetScreenWidth() / 2 - 200;
		int start_y = GetScreenHeight() / 2 - 150;
		DrawDevText(font, ("Szczegoly: " + _temp_entity_type).c_str(), start_x, start_y - 40, 24, YELLOW);
		DrawLabel(font, "Liczba sztuk:", start_x, start_y);
		if (DrawTextInput(font, _input_buffer, start_x, start_y + 20, 100, 40, _selected_field == 0)) _selected_field = 0;
		DrawLabel(font, "Spawn Radius (obszar):", start_x + 120, start_y);
		static std::string sr_buf = "5.0";
		if (DrawTextInput(font, sr_buf, start_x + 120, start_y + 20, 120, 40, _selected_field == 1)) _selected_field = 1;
		DrawLabel(font, "Trigger Radius (aktywacja):", start_x + 260, start_y);
		static std::string tr_buf = "15.0";
		if (DrawTextInput(font, tr_buf, start_x + 260, start_y + 20, 120, 40, _selected_field == 2)) _selected_field = 2;
		if (DrawButton(font, "ZAPISZ SPAWNER", start_x, start_y + 100, 400, 50, GREEN)) {
			try {
				_temp_count = std::stoi(_input_buffer.empty() ? "1" : _input_buffer);
				_temp_spawn_radius = std::stof(sr_buf);
				_temp_trigger_radius = std::stof(tr_buf);
				saveObject("spawners");
				_current_mode = EditorMode::None;
			} catch (...) { Core::Logger::errorLog("Bledne dane numeryczne w spawnerze!"); }
		}
		if (DrawButton(font, "WSTECZ", start_x, start_y + 160, 400, 40, GRAY)) _current_mode = EditorMode::SpawnerType;
	}

	void DevLevel::renderChestDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		int start_x = GetScreenWidth() / 2 - 200;
		int start_y = GetScreenHeight() / 2 - 150;
		DrawDevText(font, "Konfiguracja Skrzyni:", start_x, start_y - 40, 24, YELLOW);
		DrawLabel(font, "Nazwa:", start_x, start_y);
		if (DrawTextInput(font, _temp_name, start_x, start_y + 20, 400, 40, _selected_field == 0)) _selected_field = 0;
		std::string loot_text = "Loot IDs: ";
		for (int id : _temp_loot_ids) loot_text += std::to_string(id) + ", ";
		DrawDevText(font, loot_text.c_str(), start_x, start_y + 70, 18, LIGHTGRAY);
		if (DrawButton(font, "+ DODAJ PRZEDMIOT", start_x, start_y + 100, 400, 40, BLUE)) _current_mode = EditorMode::ItemSelection;
		if (DrawButton(font, "ZAPISZ SKRZYNIE", start_x, start_y + 160, 400, 50, GREEN)) { saveObject("chests"); _current_mode = EditorMode::None; }
		if (DrawButton(font, "WSTECZ", start_x, start_y + 220, 400, 40, GRAY)) _current_mode = EditorMode::MainMenu;
	}

	void DevLevel::renderNPCSelectionMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		int start_x = GetScreenWidth() / 2 - 150;
		int start_y = GetScreenHeight() / 2 - 100;
		DrawDevText(font, "Wybierz NPC:", start_x, start_y - 40, 24, YELLOW);
		if (DrawButton(font, "KOT (Cat)", start_x, start_y, 300, 50, BLUE)) {
			_temp_entity_type = "npc"; _temp_name = "Kot"; _temp_target_location = "cat"; saveObject("npcs"); _current_mode = EditorMode::None;
		}
		if (DrawButton(font, "WSTECZ", start_x, start_y + 80, 300, 50, GRAY)) _current_mode = EditorMode::MainMenu;
	}

	void DevLevel::renderPropDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		int start_x = GetScreenWidth() / 2 - 200;
		int start_y = GetScreenHeight() / 2 - 150;
		DrawDevText(font, "Konfiguracja Propa:", start_x, start_y - 40, 24, YELLOW);
		DrawLabel(font, "Nazwa (ID w fabryce):", start_x, start_y);
		if (DrawTextInput(font, _temp_name, start_x, start_y + 20, 400, 40, _selected_field == 0)) _selected_field = 0;
		DrawLabel(font, "Texture Path:", start_x, start_y + 70);
		static std::string tex_path = "assets/textures/chest.png";
		if (DrawTextInput(font, tex_path, start_x, start_y + 90, 400, 40, _selected_field == 1)) _selected_field = 1;
		if (DrawButton(font, "ZAPISZ PROP", start_x, start_y + 150, 400, 50, GREEN)) { _temp_target_location = tex_path; saveObject("props"); _current_mode = EditorMode::None; }
		if (DrawButton(font, "WSTECZ", start_x, start_y + 210, 400, 40, GRAY)) _current_mode = EditorMode::MainMenu;
	}

	void DevLevel::renderTeleportDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		int start_x = GetScreenWidth() / 2 - 200;
		int start_y = GetScreenHeight() / 2 - 150;
		DrawDevText(font, "Konfiguracja Teleportu:", start_x, start_y - 40, 24, YELLOW);
		DrawLabel(font, "Nazwa:", start_x, start_y);
		if (DrawTextInput(font, _temp_name, start_x, start_y + 20, 400, 40, _selected_field == 0)) _selected_field = 0;
		DrawLabel(font, "Target Location (nazwa mapy):", start_x, start_y + 70);
		if (DrawTextInput(font, _temp_target_location, start_x, start_y + 90, 400, 40, _selected_field == 1)) _selected_field = 1;
		if (DrawButton(font, "ZAPISZ TELEPORT", start_x, start_y + 150, 400, 50, GREEN)) { saveObject("teleports"); _current_mode = EditorMode::None; }
		if (DrawButton(font, "WSTECZ", start_x, start_y + 210, 400, 40, GRAY)) _current_mode = EditorMode::MainMenu;
	}

	void DevLevel::renderItemSelectionMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		int start_x = GetScreenWidth() / 2 - 250;
		int start_y = GetScreenHeight() / 2 - 250;
		DrawDevText(font, "Wybierz przedmiot do dodania:", start_x, start_y - 40, 24, YELLOW);
		auto& db = engine->getItemDatabase();
		int i = 0;
		for (const auto& [id, item] : db.getAllTemplates()) {
			std::string btn_text = std::to_string(id) + ": " + item->getName();
			if (DrawButton(font, btn_text.c_str(), start_x, start_y + i * 45, 500, 40, DARKBLUE)) { _temp_loot_ids.push_back(id); _current_mode = EditorMode::ChestDetails; }
			i++; if (i > 10) break;
		}
		if (DrawButton(font, "WSTECZ", start_x, start_y + 500, 500, 40, GRAY)) _current_mode = EditorMode::ChestDetails;
	}

	void DevLevel::resetEditorState() {
		_temp_entity_type = ""; _temp_name = ""; _temp_count = 1; _temp_spawn_radius = 5.0f; _temp_trigger_radius = 15.0f; _temp_loot_ids.clear(); _temp_target_location = ""; _input_buffer = "1"; _selected_field = 0;
	}

	void DevLevel::saveObject(const std::string& category) {
		PlacedObject po;
		po.category = category;
		po.name = _temp_name.empty() ? (_temp_entity_type.empty() ? category : _temp_entity_type) : _temp_name;
		po.type = _temp_entity_type;
		po.position = _saved_world_position;
		po.spawn_radius = _temp_spawn_radius;
		po.trigger_radius = _temp_trigger_radius;
		po.count = _temp_count;
		po.loot_ids = _temp_loot_ids;
		po.extra_string = _temp_target_location;
		_placed_objects.push_back(po);
		rewriteJsonFiles();
	}

	void DevLevel::rewriteJsonFiles() {
		std::vector<std::string> categories = { "spawners", "chests", "npcs", "props", "teleports" };
		for (const auto& cat : categories) {
			json data = json::array();
			for (const auto& obj : _placed_objects) {
				if (obj.category != cat) continue;
				json j;
				j["x"] = obj.position.x;
				j["y"] = obj.position.y;
				j["name"] = obj.name;
				j["type"] = obj.type;
				if (cat == "spawners") {
					j["count"] = obj.count;
					j["spawn_radius"] = obj.spawn_radius;
					j["trigger_radius"] = obj.trigger_radius;
				} else if (cat == "chests") {
					j["items"] = obj.loot_ids;
				} else if (cat == "npcs") {
					j["npc_class"] = obj.extra_string;
				} else if (cat == "props") {
					j["texture"] = obj.extra_string;
				} else if (cat == "teleports") {
					j["target_location"] = obj.extra_string;
				}
				data.push_back(j);
			}
			const auto output_path = resolveAssetPath("data/dev/new_level_" + cat + ".json");
			if (!output_path.parent_path().empty()) std::filesystem::create_directories(output_path.parent_path());
			std::ofstream out(output_path);
			if (out.is_open()) out << data.dump(4);
		}
	}

} // namespace Nawia::World
