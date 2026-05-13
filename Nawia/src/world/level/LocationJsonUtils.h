#pragma once

#include <json.hpp>
#include <raylib.h>

#include <filesystem>
#include <string>
#include <string_view>

namespace Nawia::World::LocationJsonUtils {

	[[nodiscard]] std::string toPathString(const std::filesystem::path& path);

	[[nodiscard]] std::string displayNameFromStem(std::string stem);

	[[nodiscard]] bool readJsonFile(
		const std::filesystem::path& path,
		nlohmann::json& output,
		std::string_view log_context
	);

	[[nodiscard]] Vector2 parseVector2(
		const nlohmann::json& data,
		Vector2 fallback = {0.0f, 0.0f}
	);

	[[nodiscard]] Vector3 parseVector3(
		const nlohmann::json& data,
		Vector3 fallback = {0.0f, 0.0f, 0.0f}
	);

	[[nodiscard]] nlohmann::json vector2ToJson(Vector2 value);
	[[nodiscard]] nlohmann::json vector3ToJson(Vector3 value);

} // namespace Nawia::World::LocationJsonUtils
