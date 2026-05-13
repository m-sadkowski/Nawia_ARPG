#include "LocationJsonUtils.h"

#include <Logger.h>

#include <cctype>
#include <fstream>

namespace Nawia::World::LocationJsonUtils {

	std::string toPathString(const std::filesystem::path& path) {
		return path.generic_string();
	}

	std::string displayNameFromStem(std::string stem) {
		for (char& character : stem) {
			if (character == '_' || character == '-')
				character = ' ';
		}

		bool capitalize_next = true;
		for (char& character : stem) {
			const auto unsigned_character = static_cast<unsigned char>(character);
			if (std::isspace(unsigned_character)) {
				capitalize_next = true;
				continue;
			}

			if (capitalize_next) {
				character = static_cast<char>(std::toupper(unsigned_character));
				capitalize_next = false;
			}
		}

		return stem;
	}

	bool readJsonFile(
		const std::filesystem::path& path,
		nlohmann::json& output,
		const std::string_view log_context
	) {
		std::ifstream file(path);
		const std::string prefix = log_context.empty() ? "JSON" : std::string(log_context);

		if (!file.is_open()) {
			Core::Logger::errorLog(prefix + ": nie mozna otworzyc pliku JSON: " + toPathString(path));
			return false;
		}

		try {
			file >> output;
		} catch (const nlohmann::json::parse_error& error) {
			Core::Logger::errorLog(prefix + ": blad parsowania JSON " + toPathString(path) + ": " + error.what());
			return false;
		}

		return true;
	}

	Vector2 parseVector2(const nlohmann::json& data, const Vector2 fallback) {
		if (data.is_object()) {
			return {
				data.value("x", fallback.x),
				data.value("y", fallback.y),
			};
		}

		if (data.is_array() && data.size() >= 2) {
			return {
				data[0].get<float>(),
				data[1].get<float>(),
			};
		}

		return fallback;
	}

	Vector3 parseVector3(const nlohmann::json& data, const Vector3 fallback) {
		if (data.is_object()) {
			return {
				data.value("x", fallback.x),
				data.value("y", fallback.y),
				data.value("z", fallback.z),
			};
		}

		if (data.is_array() && data.size() >= 3) {
			return {
				data[0].get<float>(),
				data[1].get<float>(),
				data[2].get<float>(),
			};
		}

		return fallback;
	}

	nlohmann::json vector2ToJson(const Vector2 value) {
		return {
			{"x", value.x},
			{"y", value.y},
		};
	}

	nlohmann::json vector3ToJson(const Vector3 value) {
		return {
			{"x", value.x},
			{"y", value.y},
			{"z", value.z},
		};
	}

} // namespace Nawia::World::LocationJsonUtils
