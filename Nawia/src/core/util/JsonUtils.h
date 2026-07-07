#pragma once

#include <json.hpp>

#include <initializer_list>
#include <string>

namespace Nawia::Core::JsonUtils {

	[[nodiscard]] inline std::string readStringAlias(
		const nlohmann::json& data,
		const std::initializer_list<const char*> keys)
	{
		for (const char* key : keys) {
			if (data.contains(key) && data[key].is_string())
				return data[key].get<std::string>();
		}

		return "";
	}

} // namespace Nawia::Core::JsonUtils
