#pragma once

#include <Logger.h>

#include <json.hpp>

#include <fstream>
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

	[[nodiscard]] inline nlohmann::json loadDocument(const std::string& path, const std::string& context)
	{
		std::ifstream file(path);
		if (!file.is_open()) {
			Nawia::Core::Logger::errorLog(context + ": nie mozna otworzyc JSON: " + path);
			return {};
		}

		nlohmann::json data;
		try {
			file >> data;
		} catch (const nlohmann::json::parse_error&) {
			Nawia::Core::Logger::errorLog(context + ": blad parsowania JSON: " + path);
			return {};
		}

		return data;
	}

} // namespace Nawia::Core::JsonUtils
