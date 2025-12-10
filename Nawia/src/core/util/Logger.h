#pragma once
#include <string>
#include <vector>

namespace Nawia::Core
{

	class Logger
	{
	public:
		static void debugLog(const char* message);
		static void errorLog(const char* message);
		static void debugLog(const std::string& message);
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
