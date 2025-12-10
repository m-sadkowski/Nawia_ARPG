#include "Logger.h"
#include <iostream>
#include <fstream>

namespace Nawia::Core
{
	std::vector<std::string> Logger::_logs;
	std::string Logger::_output_file_name = "logs.txt";

	void Logger::debugLog(const char* message)
	{
		internalLog("[DEBUG]", message);
	}

	void Logger::errorLog(const char* message)
	{
		internalLog("[ERROR]", message);
	}

	void Logger::debugLog(const std::string& message)
	{
		debugLog(message.c_str());
	}

	void Logger::errorLog(const std::string& message)
	{
		errorLog(message.c_str());
	}

	void Logger::internalLog(const char* prefix, const char* message)
	{
		const std::string final_message = std::string(prefix) + " " + message;

		std::cout << final_message << "\n";
		_logs.push_back(final_message);

		std::ofstream file(_output_file_name, std::ios::app);
		if (file.is_open())
		{
			file << final_message << "\n";
			file.close();
		}
	}

} // namespace Nawia::Core