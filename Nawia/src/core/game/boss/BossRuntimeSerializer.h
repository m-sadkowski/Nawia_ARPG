#pragma once

#include <BossTypes.h>
#include <json.hpp>

namespace Nawia::Game {

	class BossRuntimeSerializer {
	public:
		[[nodiscard]] static nlohmann::json toJson(const BossRuntimeState& state);
		[[nodiscard]] static bool fromJson(const nlohmann::json& data, BossRuntimeState& state);
	};

} // namespace Nawia::Game
