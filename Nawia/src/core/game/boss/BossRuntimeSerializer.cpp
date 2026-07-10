#include "BossRuntimeSerializer.h"

namespace Nawia::Game {

	nlohmann::json BossRuntimeSerializer::toJson(const BossRuntimeState& state)
	{
		nlohmann::json result;
		result["active"] = state.active;
		if (!state.active)
			return result;

		result["boss_id"] = state.boss_id;
		result["current_phase_index"] = state.current_phase_index;
		result["fight_timer"] = state.fight_timer;
		result["saved_hp"] = state.saved_hp;
		result["max_hp"] = state.max_hp;
		result["position"] = {{"x", state.position.x}, {"y", state.position.y}};
		result["altitude"] = state.altitude;
		result["spawn_position"] = {{"x", state.spawn_position.x}, {"y", state.spawn_position.y}};
		result["spawn_altitude"] = state.spawn_altitude;
		return result;
	}

	bool BossRuntimeSerializer::fromJson(const nlohmann::json& data, BossRuntimeState& state)
	{
		if (!data.is_object() || !data.value("active", false))
			return false;

		const auto read_position = [](const nlohmann::json& object) {
			return Vector2{object.value("x", 0.0f), object.value("y", 0.0f)};
		};

		state = {};
		state.active = true;
		state.boss_id = data.value("boss_id", "");
		state.current_phase_index = data.value("current_phase_index", 0);
		state.fight_timer = data.value("fight_timer", 0.0f);
		state.saved_hp = data.value("saved_hp", 0);
		state.max_hp = data.value("max_hp", 0);

		if (data.contains("position") && data["position"].is_object())
			state.position = read_position(data["position"]);
		if (data.contains("spawn_position") && data["spawn_position"].is_object())
			state.spawn_position = read_position(data["spawn_position"]);

		state.altitude = data.value("altitude", 0.0f);
		state.spawn_altitude = data.value("spawn_altitude", 0.0f);
		return true;
	}

} // namespace Nawia::Game
