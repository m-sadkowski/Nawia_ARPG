#include "SpawnManager.h"
#include "EntityFactory.h"

#include <Engine.h>
#include <Map.h>
#include <Logger.h>
#include <Entity.h>

#include <json.hpp>
#include <fstream>
#include <cmath>

using json = nlohmann::json;

namespace Nawia::World {

	bool SpawnManager::loadFromJson(const std::string& path, Core::Engine* engine,
		Core::Map* map, const std::string& initial_location) {
		std::ifstream file(path);
		if (!file.is_open()) {
			Core::Logger::errorLog("SpawnManager: nie mozna otworzyc pliku: " + path);
			return false;
		}

		json root;
		try {
			file >> root;
		} catch (const json::parse_error& e) {
			Core::Logger::errorLog("SpawnManager: blad parsowania JSON: " + std::string(e.what()));
			return false;
		}

		_spawn_points.clear();
		_player_spawns.clear();

		// Parse player spawn positions
		if (root.contains("player_spawn")) {
			for (const auto& [loc_name, pos_data] : root["player_spawn"].items()) {
				const float x = pos_data.value("x", 0.0f);
				const float y = pos_data.value("y", 0.0f);
				_player_spawns[loc_name] = {x, y};
			}
		}

		// Parse entity definitions
		if (!root.contains("entities") || !root["entities"].is_array()) {
			Core::Logger::debugLog("SpawnManager: brak tablicy 'entities' w pliku: " + path);
			return true;
		}

		for (const auto& entry : root["entities"]) {
			SpawnPoint sp;

			sp.location = entry.value("location", "");
			sp.entity_type = entry.value("type", "");
			sp.entity_data = entry;

			sp.spawn_center.x = entry.value("x", 0.0f);
			sp.spawn_center.y = entry.value("y", 0.0f);
			sp.trigger_radius = entry.value("trigger_radius", 0.0f);
			sp.spawn_radius = entry.value("spawn_radius", 0.0f);
			sp.respawnable = entry.value("respawnable", false);
			sp.respawn_cooldown = entry.value("respawn_cooldown", 0.0f);

			if (sp.entity_type.empty()) {
				Core::Logger::errorLog("SpawnManager: spawn point bez 'type' — pominieto");
				continue;
			}

			// ════════════════════════════════════════════════════════
			// PRE-CREATE ENTITY (all heavy work happens here, at load)
			// ════════════════════════════════════════════════════════

			json spawn_data = sp.entity_data;

			// Randomize position if spawn_radius set
			if (sp.spawn_radius > 0.0f) {
				const float angle = static_cast<float>(GetRandomValue(0, 360)) * DEG2RAD;
				const float dist = static_cast<float>(GetRandomValue(0, static_cast<int>(sp.spawn_radius * 100))) / 100.0f;
				spawn_data["x"] = sp.spawn_center.x + cosf(angle) * dist;
				spawn_data["y"] = sp.spawn_center.y + sinf(angle) * dist;
			}

			auto entity = EntityFactory::create(sp.entity_type, spawn_data, engine, map);
			if (!entity) {
				Core::Logger::errorLog("SpawnManager: nie udalo sie stworzyc: " + sp.entity_type);
				continue;
			}

			// Determine initial dormant state:
			// - Entities in a different location → always dormant (activated on location change)
			// - Entities in current location with trigger_radius > 0 → dormant (proximity)
			// - Entities in current location with trigger_radius == 0 → active immediately
			const bool is_current_location = (sp.location == initial_location);
			const bool should_be_active = is_current_location && (sp.trigger_radius <= 0.0f);

			entity->setDormant(!should_be_active);
			sp.activated = should_be_active;

			sp.entity = entity;
			engine->getEntityManager().addEntity(entity);

			_spawn_points.push_back(std::move(sp));
		}

		Core::Logger::debugLog("SpawnManager: zaladowano i stworzono " + 
			std::to_string(_spawn_points.size()) + " encji z " + path);
		return true;
	}

	void SpawnManager::update(
		const Vector2 player_pos,
		const std::string& current_location)
	{
		for (auto& sp : _spawn_points) {
			// Skip if not in current location
			if (sp.location != current_location) continue;

			// Skip already activated (no trigger_radius entities are already active)
			if (sp.activated) continue;

			// Skip if entity was somehow lost
			if (!sp.entity) continue;

			// Check proximity
			if (sp.trigger_radius > 0.0f) {
				const float dx = player_pos.x - sp.spawn_center.x;
				const float dy = player_pos.y - sp.spawn_center.y;
				const float dist_sq = dx * dx + dy * dy;
				const float trigger_sq = sp.trigger_radius * sp.trigger_radius;

				if (dist_sq > trigger_sq) continue; // Player too far
			}

			// ═══ WAKE UP ═══
			sp.entity->setDormant(false);
			sp.activated = true;

			Core::Logger::debugLog("SpawnManager: aktywowano " + sp.entity_type + 
				" w lokacji " + sp.location);
		}
	}

	void SpawnManager::updateLocationChange(const std::string& new_location) {
		for (auto& sp : _spawn_points) {
			if (!sp.entity) continue;

			if (sp.location == new_location) {
				// We entered this location. Wake up immediate spawns and entities
				// that had already been activated before leaving this location.
				if (sp.trigger_radius <= 0.0f || sp.activated) {
					sp.entity->setDormant(false);
					sp.activated = true;
				} else {
					// Never activated before: stay dormant until proximity wake-up.
					sp.entity->setDormant(true);
				}
			} else {
				// We left this location. Freeze the entity but remember whether it
				// had already been activated, so it can restore that state on return.
				sp.entity->setDormant(true);
			}
		}
	}

	void SpawnManager::reset() {
		for (auto& sp : _spawn_points) {
			sp.reset();
		}
		_spawn_points.clear();
	}

	bool SpawnManager::getPlayerSpawn(const std::string& location_name, Vector2& out_pos) const {
		auto it = _player_spawns.find(location_name);
		if (it != _player_spawns.end()) {
			out_pos = it->second;
			return true;
		}
		return false;
	}

} // namespace Nawia::World
