#include "Logger.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

namespace Nawia::Core {
	namespace {
		bool isDebugLoggingEnabled() {
			static const bool enabled = [] {
				const char* value = std::getenv("NAWIA_DEBUG_LOGS");
				return value != nullptr &&
					std::strcmp(value, "0") != 0 &&
					std::strcmp(value, "false") != 0 &&
					std::strcmp(value, "FALSE") != 0;
			}();

			return enabled;
		}

		bool isErrorPrefix(const char* prefix) {
			return std::strcmp(prefix, "[ERROR]") == 0;
		}
	}

	std::vector<std::string> Logger::_logs;
	std::string Logger::_output_file_name = "logs.txt";

	void Logger::debugLog(const char* message) {
		if (isDebugLoggingEnabled())
			internalLog("[DEBUG]", message);
	}

	void Logger::errorLog(const char* message) {
		internalLog("[ERROR]", message);
	}

	void Logger::debugLog(const std::string& message) {
		debugLog(message.c_str());
	}

	void Logger::errorLog(const std::string& message) {
		errorLog(message.c_str());
	}

	void Logger::internalLog(const char* prefix, const char* message) {
		const std::string final_message = std::string(prefix) + " " + message;
		const bool is_error = isErrorPrefix(prefix);

		std::cout << final_message << "\n";
		_logs.push_back(final_message);

		static std::ofstream file(_output_file_name, std::ios::app);
		if (file.is_open()) {
			file << final_message << "\n";
			if (is_error)
				file.flush();
		}
	}

} // namespace Nawia::Core
