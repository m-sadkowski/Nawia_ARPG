#include "DevLevel.h"

#include <Engine.h>
#include <EntityFactory.h>
#include <ItemDatabase.h>
#include <Logger.h>
#include <Map.h>
#include <MathUtils.h>
#include <Player.h>
#include <UIHandler.h>

#include <json.hpp>
#include <raymath.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>

using json = nlohmann::json;

namespace Nawia::World {

	namespace {

		constexpr const char* MAP_FILE = "forest.glb";
		constexpr float MAP_SCALE = 2.0f;
		constexpr float LIGHT_MOVE_STEP = 1.0f;
		constexpr int PRIMARY_LIGHT_INDEX = 0;
		constexpr int OVERLAY_MARGIN = 10;
		constexpr float OVERLAY_BACKGROUND_ALPHA = 0.85f;
		constexpr Vector2 PLAYER_SPAWN = {-4.3f, 33.0f};
		constexpr int MAX_VISIBLE_ITEMS = 11;

		constexpr std::array<std::string_view, 5> OBJECT_CATEGORIES = {
			"spawners",
			"chests",
			"npcs",
			"props",
			"teleports",
		};

		std::filesystem::path resolveAssetPath(const std::filesystem::path& relative_asset_path) {
			return (std::filesystem::path("assets") / relative_asset_path).lexically_normal();
		}

		std::string toPathString(const std::filesystem::path& path) {
			return path.generic_string();
		}

		void drawDevText(const Font& font, const char* text, float x, float y, float size, Color color) {
			DrawTextEx(font, text, {x, y}, size, 1.0f, color);
		}

		bool drawButton(const Font& font, const char* text, int x, int y, int width, int height, Color base_color) {
			const Rectangle rect = {
				static_cast<float>(x),
				static_cast<float>(y),
				static_cast<float>(width),
				static_cast<float>(height),
			};
			const bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);

			DrawRectangleRec(rect, hovered ? ColorAlpha(base_color, 0.8f) : base_color);
			DrawRectangleLinesEx(rect, 2, LIGHTGRAY);

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

		void drawLabel(const Font& font, const char* text, int x, int y) {
			drawDevText(font, text, static_cast<float>(x), static_cast<float>(y), 18, LIGHTGRAY);
		}

		bool drawTextInput(const Font& font, std::string& buffer, int x, int y, int width, int height, bool active) {
			const Rectangle rect = {
				static_cast<float>(x),
				static_cast<float>(y),
				static_cast<float>(width),
				static_cast<float>(height),
			};

			DrawRectangleRec(rect, active ? DARKGRAY : BLACK);
			DrawRectangleLinesEx(rect, 2, active ? GREEN : GRAY);
			drawDevText(font, buffer.c_str(), x + 5.0f, y + height / 2.0f - 10.0f, 20, RAYWHITE);

			if (active) {
				int key = GetCharPressed();
				while (key > 0) {
					if (key >= 32 && key <= 125) {
						buffer += static_cast<char>(key);
					}
					key = GetCharPressed();
				}

				if (IsKeyPressed(KEY_BACKSPACE) && !buffer.empty()) {
					buffer.pop_back();
				}
			}

			return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rect);
		}

		Color getCategoryColor(const std::string& category) {
			if (category == "spawners") return RED;
			if (category == "chests") return GOLD;
			if (category == "npcs") return SKYBLUE;
			if (category == "props") return GREEN;
			if (category == "teleports") return PURPLE;
			return GRAY;
		}

	} // namespace

	void DevLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu DevLevel (kreator poziomu)...");

		_map = std::make_unique<Core::Map>(engine->getResourceManager());
		_map->loadMap(MAP_FILE, MAP_SCALE, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});

		const auto lighting_file_path = resolveAssetPath("maps/forest_lighting.json");
		engine->getLightingSystem().loadLightingFromJson(toPathString(lighting_file_path));

		auto& entity_manager = engine->getEntityManager();
		entity_manager.clearNonPlayerEntities();

		if (const auto player = engine->getPlayer()) {
			const Vector3 snapped_spawn =
				_map->getNavMesh().getClosestWalkablePosition({PLAYER_SPAWN.x, 0.0f, PLAYER_SPAWN.y});
			player->setX(snapped_spawn.x);
			player->setY(snapped_spawn.z);
			player->setAltitude(snapped_spawn.y);
			player->setRespawnPoint({snapped_spawn.x, snapped_spawn.z});
			player->stop();
		}

		loadPlacedObjects();
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
			float movement_speed = 5.0f;
			if (IsKeyDown(KEY_LEFT_SHIFT)) {
				movement_speed = 25.0f;
			}

			player->setMovementSpeed(movement_speed);

			if (_current_mode == EditorMode::None) {
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
		if (!engine || engine->isPaused()) {
			return;
		}

		renderPlacedObjects(engine);
		renderLightingOverlay(*engine);

		if (_current_mode == EditorMode::None) {
			return;
		}

		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, OVERLAY_BACKGROUND_ALPHA));

		switch (_current_mode) {
			case EditorMode::MainMenu:
				renderMainMenu(engine);
				break;
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
			case EditorMode::ItemSelection:
				renderItemSelectionMenu(engine);
				break;
			case EditorMode::None:
				break;
		}
	}

	void DevLevel::loadPlacedObjects() {
		_placed_objects.clear();

		for (const std::string_view category : OBJECT_CATEGORIES) {
			const std::string category_name(category);
			const auto file_path = getCategoryFilePath(category_name);
			if (std::filesystem::exists(file_path)) {
				loadPlacedObjectsFromFile(category_name, file_path);
			}
		}
	}

	void DevLevel::loadPlacedObjectsFromFile(const std::string& category, const std::filesystem::path& path) {
		std::ifstream file(path);
		if (!file.is_open()) {
			Core::Logger::errorLog("DevLevel: nie mozna otworzyc pliku: " + toPathString(path));
			return;
		}

		json data;
		try {
			file >> data;
		} catch (const json::parse_error& error) {
			Core::Logger::errorLog("DevLevel: blad parsowania JSON: " + std::string(error.what()));
			return;
		}

		if (!data.is_array()) {
			Core::Logger::errorLog("DevLevel: plik kategorii nie jest tablica: " + toPathString(path));
			return;
		}

		for (const auto& entry : data) {
			_placed_objects.push_back(parsePlacedObject(category, entry));
		}
	}

	DevLevel::PlacedObject DevLevel::parsePlacedObject(const std::string& category, const json& data) const {
		PlacedObject placed_object;
		placed_object.category = category;
		placed_object.name = data.value("name", "Unknown");
		placed_object.type = data.value("type", "");
		placed_object.position = {data.value("x", 0.0f), data.value("y", 0.0f)};
		placed_object.spawn_radius = data.value("spawn_radius", 0.0f);
		placed_object.trigger_radius = data.value("trigger_radius", 0.0f);
		placed_object.count = data.value("count", 1);

		if (data.contains("items") && data["items"].is_array()) {
			for (const auto& item_id : data["items"]) {
				placed_object.loot_ids.push_back(item_id.get<int>());
			}
		}

		if (category == "npcs") {
			placed_object.extra_value = data.value("npc_class", "");
		} else if (category == "props") {
			placed_object.extra_value = data.value("texture", "");
		} else if (category == "teleports") {
			placed_object.extra_value = data.value("target_location", "");
		}

		return placed_object;
	}

	json DevLevel::serializePlacedObject(const PlacedObject& object) const {
		json data;
		data["x"] = object.position.x;
		data["y"] = object.position.y;
		data["name"] = object.name;
		data["type"] = object.type;

		if (object.category == "spawners") {
			data["count"] = object.count;
			data["spawn_radius"] = object.spawn_radius;
			data["trigger_radius"] = object.trigger_radius;
		} else if (object.category == "chests") {
			data["items"] = object.loot_ids;
		} else if (object.category == "npcs") {
			data["npc_class"] = object.extra_value;
		} else if (object.category == "props") {
			data["texture"] = object.extra_value;
		} else if (object.category == "teleports") {
			data["target_location"] = object.extra_value;
		}

		return data;
	}

	std::filesystem::path DevLevel::getCategoryFilePath(const std::string& category) const {
		return resolveAssetPath(std::filesystem::path("data/dev") / ("new_level_" + category + ".json"));
	}

	void DevLevel::handleUIInput(Core::Engine* engine) {
		(void)engine;

		if (!IsKeyPressed(KEY_ESCAPE)) {
			return;
		}

		if (_current_mode == EditorMode::MainMenu) {
			_current_mode = EditorMode::None;
		} else {
			_current_mode = EditorMode::MainMenu;
		}
	}

	void DevLevel::handleEditingInput(Core::Engine* engine) {
		if (!engine) {
			return;
		}

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

			if (lighting_changed) {
				lighting_system.updateLightValues(PRIMARY_LIGHT_INDEX);
			}

			if (IsKeyPressed(KEY_S)) {
				const auto lighting_file_path = resolveAssetPath("maps/forest_lighting.json");
				lighting_system.saveLightingToJson(toPathString(lighting_file_path));
				Core::Logger::debugLog("DevLevel: zapisano oswietlenie.");
			}
		}

		if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
			return;
		}

		if (engine->getUIHandler().isMouseOverUI() || engine->getUIHandler().isInputBlocked()) {
			return;
		}

		const Vector2 mouse_position = GetMousePosition();
		const Ray ray = GetScreenToWorldRay(mouse_position, engine->getCamera().get());
		const RayCollision collision = _map ? _map->getRayCollision(ray) : RayCollision{};

		if (collision.hit) {
			_saved_world_position = {collision.point.x, collision.point.z};
		} else {
			_saved_world_position = Core::screenToWorld(engine->getCamera().get(), mouse_position.x, mouse_position.y);
		}

		resetEditorState();
		_current_mode = EditorMode::MainMenu;
	}

	void DevLevel::renderPlacedObjects(Core::Engine* engine) {
		if (!engine || !_map) {
			return;
		}

		const auto& camera = engine->getCamera().get();
		const auto& font = engine->getUIHandler().getFont();

		BeginMode3D(camera);

		for (const auto& object : _placed_objects) {
			const Vector3 nav_position =
				_map->getNavMesh().getClosestWalkablePosition({object.position.x, 0.0f, object.position.y});
			const Vector3 marker_position = {object.position.x, nav_position.y + 0.5f, object.position.y};
			const Color marker_color = getCategoryColor(object.category);

			DrawCube(marker_position, 1.0f, 1.0f, 1.0f, marker_color);
			DrawCubeWires(marker_position, 1.1f, 1.1f, 1.1f, RAYWHITE);

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
			}
		}

		EndMode3D();

		for (const auto& object : _placed_objects) {
			const Vector3 nav_position =
				_map->getNavMesh().getClosestWalkablePosition({object.position.x, 0.0f, object.position.y});
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
		if (_placed_objects.empty() || !engine || !_map || engine->getUIHandler().isMouseOverUI()) {
			return;
		}

		const Ray ray = GetScreenToWorldRay(GetMousePosition(), engine->getCamera().get());
		int nearest_index = -1;
		float nearest_hit_distance = std::numeric_limits<float>::max();

		for (int index = 0; index < static_cast<int>(_placed_objects.size()); ++index) {
			const auto& object = _placed_objects[index];
			const Vector3 nav_position =
				_map->getNavMesh().getClosestWalkablePosition({object.position.x, 0.0f, object.position.y});
			const Vector3 marker_position = {object.position.x, nav_position.y + 0.5f, object.position.y};
			const BoundingBox marker_box = {
				{marker_position.x - 0.6f, marker_position.y - 0.6f, marker_position.z - 0.6f},
				{marker_position.x + 0.6f, marker_position.y + 0.6f, marker_position.z + 0.6f},
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

		if (nearest_index == -1) {
			return;
		}

		Core::Logger::debugLog("DevLevel: usunieto obiekt: " + _placed_objects[nearest_index].name);
		_placed_objects.erase(_placed_objects.begin() + nearest_index);
		rewriteJsonFiles();
	}

	void DevLevel::testLevel(Core::Engine* engine) {
		if (!engine) {
			return;
		}

		auto& entity_manager = engine->getEntityManager();
		entity_manager.clearNonPlayerEntities();
		_spawn_manager.reset();

		for (const auto& object : _placed_objects) {
			json entity_data = serializePlacedObject(object);

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
					if (!entity) {
						continue;
					}

					if (_map && _map->getNavMesh().isReady()) {
						const Vector3 snapped_position =
							_map->getNavMesh().getClosestWalkablePosition({entity->getX(), 0.0f, entity->getY()});
						entity->setX(snapped_position.x);
						entity->setY(snapped_position.z);
						entity->setAltitude(snapped_position.y);
					}

					SpawnPoint spawn_point;
					spawn_point.location = "Dev Sandbox";
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
					entity_manager.addEntity(entity);
				}
			}
		}

		Core::Logger::debugLog("DevLevel: zespawnowano testowy poziom.");
	}

	void DevLevel::renderLightingOverlay(Core::Engine& engine) const {
		const auto& font = engine.getUIHandler().getFont();
		int x = OVERLAY_MARGIN;
		int y = 10;

		DrawRectangle(5, 5, 450, 240, Fade(BLACK, 0.7f));
		DrawRectangleLines(5, 5, 450, 240, DARKGRAY);

		drawDevText(font, "INSTRUKCJA KREATORA POZIOMU", x, y, 20, YELLOW);
		y += 30;
		drawDevText(font, "- Prawy klik: otwiera menu dodawania obiektu", x, y, 16, RAYWHITE);
		y += 20;
		drawDevText(font, "- SHIFT + WASD: szybkie poruszanie po mapie", x, y, 16, RAYWHITE);
		y += 20;
		drawDevText(font, "- DELETE / X: usuwa obiekt pod kursorem", x, y, 16, RED);
		y += 20;
		drawDevText(font, "- S: zapisuje oswietlenie mapy", x, y, 16, RAYWHITE);
		y += 20;
		drawDevText(font, "- Strzalki/PgUp/PgDn: sterowanie swiatlem", x, y, 16, GRAY);
		y += 30;

		drawDevText(font, "LEGENDA ZASIEGOW:", x, y, 18, YELLOW);
		y += 25;
		DrawCircle(x + 10, y + 8, 8, ColorAlpha(YELLOW, 0.5f));
		drawDevText(font, "Spawn Radius (zolty)", x + 25, y, 16, RAYWHITE);
		y += 20;
		DrawCircle(x + 10, y + 8, 8, ColorAlpha(ORANGE, 0.5f));
		drawDevText(font, "Trigger Radius (pomaranczowy)", x + 25, y, 16, RAYWHITE);
		y += 25;

		drawDevText(font, "Zapis do: assets/data/dev/new_level_*.json", x, y, 14, GREEN);
	}

	void DevLevel::renderMainMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 150;
		const int start_y = GetScreenHeight() / 2 - 200;

		drawDevText(font, "Wybierz typ obiektu:", start_x, start_y - 40, 24, YELLOW);

		if (drawButton(font, "SPAWNER (Przeciwnicy)", start_x, start_y, 300, 50, DARKBLUE)) {
			_current_mode = EditorMode::SpawnerType;
		}
		if (drawButton(font, "SKRZYNIA (Loot)", start_x, start_y + 60, 300, 50, DARKBLUE)) {
			_temp_entity_type = "chest";
			_temp_name = "Skrzynia";
			_current_mode = EditorMode::ChestDetails;
		}
		if (drawButton(font, "NPC (Rozmowa)", start_x, start_y + 120, 300, 50, DARKBLUE)) {
			_current_mode = EditorMode::NPCSelection;
		}
		if (drawButton(font, "PROP (Static Object)", start_x, start_y + 180, 300, 50, DARKBLUE)) {
			_temp_entity_type = "static_object";
			_temp_name = "Prop";
			_current_mode = EditorMode::PropDetails;
		}
		if (drawButton(font, "TELEPORT", start_x, start_y + 240, 300, 50, DARKBLUE)) {
			_temp_entity_type = "teleport";
			_temp_name = "Teleport";
			_current_mode = EditorMode::TeleportDetails;
		}

		if (drawButton(font, "TESTUJ POZIOM (Spawn)", start_x - 320, start_y, 300, 50, ORANGE)) {
			testLevel(engine);
			_current_mode = EditorMode::None;
		}
		if (drawButton(font, "ZAPISZ SESJE", start_x - 320, start_y + 60, 300, 50, GREEN)) {
			rewriteJsonFiles();
			Core::Logger::debugLog("DevLevel: sesja zapisana manualnie.");
		}
		if (drawButton(font, "ANULUJ", start_x, start_y + 320, 300, 50, MAROON)) {
			_current_mode = EditorMode::None;
		}
	}

	void DevLevel::renderSpawnerTypeMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 150;
		const int start_y = GetScreenHeight() / 2 - 150;

		drawDevText(font, "Typ przeciwnika:", start_x, start_y - 40, 24, YELLOW);

		if (drawButton(font, "Devil", start_x, start_y, 300, 40, BLUE)) {
			_temp_entity_type = "devil";
			_current_mode = EditorMode::SpawnerDetails;
		}
		if (drawButton(font, "Bandit", start_x, start_y + 50, 300, 40, BLUE)) {
			_temp_entity_type = "bandit";
			_current_mode = EditorMode::SpawnerDetails;
		}
		if (drawButton(font, "Walking Dead", start_x, start_y + 100, 300, 40, BLUE)) {
			_temp_entity_type = "walking_dead";
			_current_mode = EditorMode::SpawnerDetails;
		}
		if (drawButton(font, "WSTECZ", start_x, start_y + 180, 300, 40, GRAY)) {
			_current_mode = EditorMode::MainMenu;
		}
	}

	void DevLevel::renderSpawnerDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 200;
		const int start_y = GetScreenHeight() / 2 - 150;
		const std::string title = "Szczegoly: " + _temp_entity_type;

		drawDevText(font, title.c_str(), start_x, start_y - 40, 24, YELLOW);
		drawLabel(font, "Liczba sztuk:", start_x, start_y);
		if (drawTextInput(font, _count_buffer, start_x, start_y + 20, 100, 40, _selected_field == 0)) {
			_selected_field = 0;
		}

		drawLabel(font, "Spawn Radius (obszar):", start_x + 120, start_y);
		if (drawTextInput(font, _spawn_radius_buffer, start_x + 120, start_y + 20, 120, 40, _selected_field == 1)) {
			_selected_field = 1;
		}

		drawLabel(font, "Trigger Radius (aktywacja):", start_x + 260, start_y);
		if (drawTextInput(
				font,
				_trigger_radius_buffer,
				start_x + 260,
				start_y + 20,
				120,
				40,
				_selected_field == 2
			)) {
			_selected_field = 2;
		}

		if (drawButton(font, "ZAPISZ SPAWNER", start_x, start_y + 100, 400, 50, GREEN)) {
			try {
				_temp_count = std::max(1, std::stoi(_count_buffer.empty() ? "1" : _count_buffer));
				_temp_spawn_radius = std::max(0.0f, std::stof(_spawn_radius_buffer));
				_temp_trigger_radius = std::max(0.0f, std::stof(_trigger_radius_buffer));
				saveObject("spawners");
				_current_mode = EditorMode::None;
			} catch (const std::exception& error) {
				Core::Logger::errorLog("DevLevel: bledne dane numeryczne w spawnerze: " + std::string(error.what()));
			}
		}
		if (drawButton(font, "WSTECZ", start_x, start_y + 160, 400, 40, GRAY)) {
			_current_mode = EditorMode::SpawnerType;
		}
	}

	void DevLevel::renderChestDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 200;
		const int start_y = GetScreenHeight() / 2 - 150;

		drawDevText(font, "Konfiguracja skrzyni:", start_x, start_y - 40, 24, YELLOW);
		drawLabel(font, "Nazwa:", start_x, start_y);
		if (drawTextInput(font, _temp_name, start_x, start_y + 20, 400, 40, _selected_field == 0)) {
			_selected_field = 0;
		}

		std::string loot_text = "Loot IDs: ";
		for (const int item_id : _temp_loot_ids) {
			loot_text += std::to_string(item_id) + ", ";
		}
		drawDevText(font, loot_text.c_str(), start_x, start_y + 70, 18, LIGHTGRAY);

		if (drawButton(font, "+ DODAJ PRZEDMIOT", start_x, start_y + 100, 400, 40, BLUE)) {
			_current_mode = EditorMode::ItemSelection;
		}
		if (drawButton(font, "ZAPISZ SKRZYNIE", start_x, start_y + 160, 400, 50, GREEN)) {
			saveObject("chests");
			_current_mode = EditorMode::None;
		}
		if (drawButton(font, "WSTECZ", start_x, start_y + 220, 400, 40, GRAY)) {
			_current_mode = EditorMode::MainMenu;
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
		if (drawButton(font, "WSTECZ", start_x, start_y + 80, 300, 50, GRAY)) {
			_current_mode = EditorMode::MainMenu;
		}
	}

	void DevLevel::renderPropDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 200;
		const int start_y = GetScreenHeight() / 2 - 150;

		drawDevText(font, "Konfiguracja propa:", start_x, start_y - 40, 24, YELLOW);
		drawLabel(font, "Nazwa:", start_x, start_y);
		if (drawTextInput(font, _temp_name, start_x, start_y + 20, 400, 40, _selected_field == 0)) {
			_selected_field = 0;
		}

		drawLabel(font, "Texture Path:", start_x, start_y + 70);
		if (drawTextInput(font, _texture_path_buffer, start_x, start_y + 90, 400, 40, _selected_field == 1)) {
			_selected_field = 1;
		}

		if (drawButton(font, "ZAPISZ PROP", start_x, start_y + 150, 400, 50, GREEN)) {
			_temp_extra_value = _texture_path_buffer;
			saveObject("props");
			_current_mode = EditorMode::None;
		}
		if (drawButton(font, "WSTECZ", start_x, start_y + 210, 400, 40, GRAY)) {
			_current_mode = EditorMode::MainMenu;
		}
	}

	void DevLevel::renderTeleportDetailsMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 200;
		const int start_y = GetScreenHeight() / 2 - 150;

		drawDevText(font, "Konfiguracja teleportu:", start_x, start_y - 40, 24, YELLOW);
		drawLabel(font, "Nazwa:", start_x, start_y);
		if (drawTextInput(font, _temp_name, start_x, start_y + 20, 400, 40, _selected_field == 0)) {
			_selected_field = 0;
		}

		drawLabel(font, "Target Location (nazwa mapy):", start_x, start_y + 70);
		if (drawTextInput(font, _temp_extra_value, start_x, start_y + 90, 400, 40, _selected_field == 1)) {
			_selected_field = 1;
		}

		if (drawButton(font, "ZAPISZ TELEPORT", start_x, start_y + 150, 400, 50, GREEN)) {
			saveObject("teleports");
			_current_mode = EditorMode::None;
		}
		if (drawButton(font, "WSTECZ", start_x, start_y + 210, 400, 40, GRAY)) {
			_current_mode = EditorMode::MainMenu;
		}
	}

	void DevLevel::renderItemSelectionMenu(Core::Engine* engine) {
		const auto& font = engine->getUIHandler().getFont();
		const int start_x = GetScreenWidth() / 2 - 250;
		const int start_y = GetScreenHeight() / 2 - 250;

		drawDevText(font, "Wybierz przedmiot do dodania:", start_x, start_y - 40, 24, YELLOW);

		int row = 0;
		for (const auto& [item_id, item] : engine->getItemDatabase().getAllTemplates()) {
			if (row >= MAX_VISIBLE_ITEMS) {
				break;
			}

			const std::string button_text = std::to_string(item_id) + ": " + item->getName();
			if (drawButton(font, button_text.c_str(), start_x, start_y + row * 45, 500, 40, DARKBLUE)) {
				_temp_loot_ids.push_back(item_id);
				_current_mode = EditorMode::ChestDetails;
			}
			++row;
		}

		if (drawButton(font, "WSTECZ", start_x, start_y + 500, 500, 40, GRAY)) {
			_current_mode = EditorMode::ChestDetails;
		}
	}

	void DevLevel::resetEditorState() {
		_temp_entity_type.clear();
		_temp_name.clear();
		_temp_count = 1;
		_temp_spawn_radius = 5.0f;
		_temp_trigger_radius = 15.0f;
		_temp_loot_ids.clear();
		_temp_extra_value.clear();

		_count_buffer = "1";
		_spawn_radius_buffer = "5.0";
		_trigger_radius_buffer = "15.0";
		_texture_path_buffer = "assets/textures/chest.png";
		_selected_field = 0;
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
		placed_object.extra_value = _temp_extra_value;

		_placed_objects.push_back(std::move(placed_object));
		rewriteJsonFiles();
	}

	void DevLevel::rewriteJsonFiles() {
		for (const std::string_view category : OBJECT_CATEGORIES) {
			const std::string category_name(category);
			json data = json::array();

			for (const auto& object : _placed_objects) {
				if (object.category == category_name) {
					data.push_back(serializePlacedObject(object));
				}
			}

			const auto output_path = getCategoryFilePath(category_name);

			try {
				if (!output_path.parent_path().empty()) {
					std::filesystem::create_directories(output_path.parent_path());
				}
			} catch (const std::filesystem::filesystem_error& error) {
				Core::Logger::errorLog("DevLevel: nie mozna utworzyc katalogu: " + std::string(error.what()));
				continue;
			}

			std::ofstream output(output_path);
			if (!output.is_open()) {
				Core::Logger::errorLog("DevLevel: nie mozna zapisac pliku: " + toPathString(output_path));
				continue;
			}

			output << data.dump(4);
		}
	}

} // namespace Nawia::World
