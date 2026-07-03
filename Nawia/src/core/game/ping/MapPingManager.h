#pragma once

#include <Entity.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <raylib.h>
#include <string>
#include <utility>
#include <vector>

namespace Nawia::Game {

	enum class MapPingType {
		Info,
		Threat
	};

	struct MapPingSource {
		bool valid = false;
		std::weak_ptr<Entity::Entity> entity;
		Entity::EntityId entity_id = Entity::INVALID_ENTITY_ID;
		std::string name;
		Entity::EntityType type = Entity::EntityType::None;
		Entity::Faction faction = Entity::Faction::None;
	};

	struct MapPing {
		std::uint64_t id = 0;
		float created_time_seconds = 0.0f;
		float age_seconds = 0.0f;
		float duration_seconds = 5.0f;
		bool active = false;
		MapPingType type = MapPingType::Info;
		Vector3 position = {0.0f, 0.0f, 0.0f};
		MapPingSource source;
	};

	/**
	 * @class MapPingManager
	 * @brief Stores temporary world pings and the last ping remembered per source and type.
	 *
	 * Pings are tactical communication data, not world entities. They render for
	 * a short time, then remain available as the last known ping from their
	 * source and type for agent perception.
	 */
	class MapPingManager {
	public:
		struct Settings {
			float visible_duration_seconds = 5.0f;
			size_t max_active_pings = 24;
		};

		void update(float dt);
		void clear();
		MapPing placePing(const std::shared_ptr<Entity::Entity>& source, Vector3 position);
		MapPing placePing(const std::shared_ptr<Entity::Entity>& source, Vector3 position, MapPingType type);
		void render(const Camera3D& camera) const;

		[[nodiscard]] float getTimeSeconds() const { return _time_seconds; }
		[[nodiscard]] MapPingType getSelectedType() const { return _selected_type; }
		[[nodiscard]] const std::vector<MapPing>& getActivePings() const { return _active_pings; }
		[[nodiscard]] std::vector<MapPing> getRememberedPings() const;
		[[nodiscard]] const Settings& getSettings() const { return _settings; }
		void setSettings(const Settings& settings);
		void selectType(MapPingType type);
		void cycleSelectedType(int direction);

	private:
		using PingMemoryKey = std::pair<Entity::EntityId, MapPingType>;

		[[nodiscard]] MapPingSource makeSourceSnapshot(const std::shared_ptr<Entity::Entity>& source) const;
		[[nodiscard]] MapPing withCurrentState(MapPing ping) const;
		void removeActivePing(Entity::EntityId source_id, MapPingType type);
		void trimActivePings();

		Settings _settings;
		float _time_seconds = 0.0f;
		std::uint64_t _next_ping_id = 1;
		MapPingType _selected_type = MapPingType::Info;
		std::vector<MapPing> _active_pings;
		std::map<PingMemoryKey, MapPing> _last_ping_by_source_and_type;
	};

	[[nodiscard]] const char* toString(MapPingType type);
	[[nodiscard]] Color getPingColor(MapPingType type);

} // namespace Nawia::Game
