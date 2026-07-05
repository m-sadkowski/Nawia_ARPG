#include "EntityAbilityConfig.h"

#include <Logger.h>

#include <json.hpp>

#include <fstream>

namespace Nawia::Entity {
namespace {

	constexpr const char* ABILITIES_PATH = "assets/data/abilities.json";

	nlohmann::json loadAbilitiesData()
	{
		std::ifstream file(ABILITIES_PATH);
		if (!file.is_open()) {
			Core::Logger::errorLog(std::string("Entity - cannot open file: ") + ABILITIES_PATH);
			return {};
		}

		nlohmann::json data;
		try {
			file >> data;
		}
		catch (const nlohmann::json::parse_error&) {
			Core::Logger::errorLog(std::string("Entity - cannot parse JSON: ") + ABILITIES_PATH);
			return {};
		}

		return data;
	}

	template <typename T>
	void assignStatIfPresent(const nlohmann::json& json_stats, const char* key, T& destination)
	{
		if (const auto stat_it = json_stats.find(key); stat_it != json_stats.end())
			destination = stat_it->get<T>();
	}

}

	AbilityStats getAbilityStatsFromConfig(const std::string& name)
	{
		static const nlohmann::json data = loadAbilitiesData();

		if (data.contains("abilities")) {
			for (const auto& ability : data["abilities"]) {
				if (ability.value("name", "") == name) {
					AbilityStats stats;
					if (const auto stats_it = ability.find("stats"); stats_it != ability.end() && stats_it->is_object()) {
						const auto& json_stats = *stats_it;
						assignStatIfPresent(json_stats, "damage", stats.damage);
						assignStatIfPresent(json_stats, "cooldown", stats.cooldown);
						assignStatIfPresent(json_stats, "cast_range", stats.cast_range);
						assignStatIfPresent(json_stats, "projectile_speed", stats.projectile_speed);
						assignStatIfPresent(json_stats, "duration", stats.duration);
						assignStatIfPresent(json_stats, "hitbox_radius", stats.hitbox_radius);
					}
					return stats;
				}
			}
		}

		Core::Logger::errorLog("Entity - ability not found: " + name);
		return {};
	}

}
