#pragma once

#include <BossTypes.h>

#include <map>
#include <string>

namespace Nawia::Game {

	class BossDefinitionLoader {
	public:
		[[nodiscard]] static std::map<std::string, BossData> loadFromJson(const std::string& path);
	};

} // namespace Nawia::Game
