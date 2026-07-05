#pragma once

#include <AbilityStats.h>

#include <string>

namespace Nawia::Entity {

	[[nodiscard]] AbilityStats getAbilityStatsFromConfig(const std::string& name);

}
