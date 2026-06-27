#include "MapPingManager.h"

#include <algorithm>
#include <cmath>

namespace Nawia::Game {

	namespace {
		[[nodiscard]] Entity::EntityId entityId(const Entity::Entity* entity) {
			return entity ? entity->getEntityId() : Entity::INVALID_ENTITY_ID;
		}

		[[nodiscard]] bool canCreatePing(const std::shared_ptr<Entity::Entity>& source) {
			if (!source || !source->isPerceptionVisible())
				return false;

			const Entity::EntityType type = source->getType();
			return type == Entity::EntityType::Player || type == Entity::EntityType::Ally;
		}
	}

	void MapPingManager::update(const float dt) {
		_time_seconds += std::max(0.0f, dt);
		for (auto& ping : _active_pings)
			ping = withCurrentState(ping);

		_active_pings.erase(
			std::remove_if(_active_pings.begin(), _active_pings.end(), [](const MapPing& ping) {
				return !ping.active;
			}),
			_active_pings.end());
	}

	void MapPingManager::clear() {
		_time_seconds = 0.0f;
		_next_ping_id = 1;
		_active_pings.clear();
		_last_ping_by_source_and_type.clear();
	}

	MapPing MapPingManager::placePing(const std::shared_ptr<Entity::Entity>& source, const Vector3 position) {
		return placePing(source, position, _selected_type);
	}

	MapPing MapPingManager::placePing(
		const std::shared_ptr<Entity::Entity>& source,
		const Vector3 position,
		const MapPingType type)
	{
		MapPing ping;
		if (!canCreatePing(source))
			return ping;

		ping.source = makeSourceSnapshot(source);
		if (!ping.source.valid)
			return {};

		removeActivePing(ping.source.entity_id, type);

		ping.id = _next_ping_id++;
		ping.created_time_seconds = _time_seconds;
		ping.duration_seconds = std::max(0.1f, _settings.visible_duration_seconds);
		ping.active = true;
		ping.type = type;
		ping.position = position;

		_active_pings.push_back(ping);
		_last_ping_by_source_and_type[{ping.source.entity_id, ping.type}] = ping;
		trimActivePings();
		return ping;
	}

	void MapPingManager::render(const Camera3D& /*camera*/) const {
		for (const auto& ping : _active_pings) {
			const float duration = std::max(0.1f, ping.duration_seconds);
			const float progress = std::clamp(ping.age_seconds / duration, 0.0f, 1.0f);
			const float alpha = std::clamp(1.0f - progress, 0.0f, 1.0f);
			const float pulse = 0.07f * std::sin((ping.age_seconds + 0.1f) * 8.0f);
			const float radius = 0.45f + pulse;
			const Color base = getPingColor(ping.type);
			const Vector3 ground = {ping.position.x, ping.position.y + 0.05f, ping.position.z};
			const Vector3 top = {ping.position.x, ping.position.y + 1.25f, ping.position.z};

			DrawCylinder(ground, radius, radius, 0.035f, 36, Fade(base, 0.20f * alpha));
			DrawCylinderWires(ground, radius, radius, 0.08f, 36, Fade(base, 0.95f * alpha));
			DrawLine3D(ground, top, Fade(base, 0.85f * alpha));
			DrawSphere(top, 0.10f, Fade(base, 0.95f * alpha));

			if (ping.type == MapPingType::Threat) {
				const float cross_radius = radius * 0.78f;
				const Vector3 left = {ground.x - cross_radius, ground.y + 0.03f, ground.z - cross_radius};
				const Vector3 right = {ground.x + cross_radius, ground.y + 0.03f, ground.z + cross_radius};
				const Vector3 front = {ground.x - cross_radius, ground.y + 0.03f, ground.z + cross_radius};
				const Vector3 back = {ground.x + cross_radius, ground.y + 0.03f, ground.z - cross_radius};
				DrawLine3D(left, right, Fade(base, 0.90f * alpha));
				DrawLine3D(front, back, Fade(base, 0.90f * alpha));
			}
		}
	}

	std::vector<MapPing> MapPingManager::getRememberedPings() const {
		std::vector<MapPing> pings;
		pings.reserve(_last_ping_by_source_and_type.size());
		for (const auto& [_, ping] : _last_ping_by_source_and_type)
			pings.push_back(withCurrentState(ping));

		std::sort(pings.begin(), pings.end(), [](const MapPing& left, const MapPing& right) {
			return left.created_time_seconds > right.created_time_seconds;
		});
		return pings;
	}

	void MapPingManager::setSettings(const Settings& settings) {
		_settings = settings;
		_settings.visible_duration_seconds = std::max(0.1f, _settings.visible_duration_seconds);
		_settings.max_active_pings = std::max<size_t>(1, _settings.max_active_pings);
		trimActivePings();
	}

	void MapPingManager::selectType(const MapPingType type) {
		_selected_type = type;
	}

	void MapPingManager::cycleSelectedType(const int direction) {
		if (direction == 0)
			return;

		_selected_type = _selected_type == MapPingType::Info ? MapPingType::Threat : MapPingType::Info;
	}

	MapPingSource MapPingManager::makeSourceSnapshot(const std::shared_ptr<Entity::Entity>& source) const {
		MapPingSource snapshot;
		if (!source)
			return snapshot;

		snapshot.valid = true;
		snapshot.entity = source;
		snapshot.entity_id = entityId(source.get());
		snapshot.name = source->getName();
		snapshot.type = source->getType();
		snapshot.faction = source->getFaction();
		return snapshot;
	}

	MapPing MapPingManager::withCurrentState(MapPing ping) const {
		ping.age_seconds = std::max(0.0f, _time_seconds - ping.created_time_seconds);
		ping.active = ping.age_seconds <= ping.duration_seconds;
		return ping;
	}

	void MapPingManager::removeActivePing(const Entity::EntityId source_id, const MapPingType type) {
		_active_pings.erase(
			std::remove_if(_active_pings.begin(), _active_pings.end(), [source_id, type](const MapPing& ping) {
				return ping.source.entity_id == source_id && ping.type == type;
			}),
			_active_pings.end());
	}

	void MapPingManager::trimActivePings() {
		while (_active_pings.size() > _settings.max_active_pings)
			_active_pings.erase(_active_pings.begin());
	}

	const char* toString(const MapPingType type) {
		switch (type) {
			case MapPingType::Info:
				return "Info";
			case MapPingType::Threat:
				return "Threat";
		}

		return "Unknown";
	}

	Color getPingColor(const MapPingType type) {
		switch (type) {
			case MapPingType::Info:
				return Color{80, 190, 255, 255};
			case MapPingType::Threat:
				return Color{235, 70, 70, 255};
		}

		return WHITE;
	}

} // namespace Nawia::Game
