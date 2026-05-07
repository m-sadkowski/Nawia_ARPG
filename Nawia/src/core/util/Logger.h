#pragma once

#include <string>
#include <vector>

namespace Nawia::Core {

	/**
	 * @class Logger
	 * @brief Prosty logger konsolowy i plikowy.
	 */
	class Logger {
	public:
		/** @brief Zapisuje komunikat debugowy. */
		static void debugLog(const char* message);

		/** @brief Zapisuje komunikat bledu. */
		static void errorLog(const char* message);

		/** @brief Zapisuje komunikat debugowy. */
		static void debugLog(const std::string& message);

		/** @brief Zapisuje komunikat bledu. */
		static void errorLog(const std::string& message);

	private:
		Logger() = delete;
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;

		static std::vector<std::string> _logs;
		static std::string _output_file_name;

		static void internalLog(const char* prefix, const char* message);
	};

} // namespace Nawia::Core
