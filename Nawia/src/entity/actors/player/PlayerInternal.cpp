#include "PlayerInternal.h"

namespace Nawia::Entity::PlayerDetail {

	nlohmann::json statsToJson(const Stats& stats) {
		return {
			{"max_hp", stats.max_hp},
			{"damage", stats.damage},
			{"power", stats.power},
			{"attack_speed", stats.attack_speed},
			{"movement_speed", stats.movement_speed},
			{"defense", stats.defense}
		};
	}

	Stats statsFromJson(const nlohmann::json& data, const Stats& fallback) {
		if (!data.is_object())
			return fallback;

		Stats stats = fallback;
		stats.max_hp = data.value("max_hp", stats.max_hp);
		stats.damage = data.value("damage", stats.damage);
		stats.power = data.value("power", stats.power);
		stats.attack_speed = data.value("attack_speed", stats.attack_speed);
		stats.movement_speed = data.value("movement_speed", stats.movement_speed);
		stats.defense = data.value("defense", stats.defense);
		return stats;
	}

	nlohmann::json vector2ToJson(const Vector2 value) {
		return {{"x", value.x}, {"y", value.y}};
	}

	Vector2 vector2FromJson(const nlohmann::json& data, const Vector2 fallback) {
		if (!data.is_object())
			return fallback;

		return {data.value("x", fallback.x), data.value("y", fallback.y)};
	}

} // namespace Nawia::Entity::PlayerDetail
