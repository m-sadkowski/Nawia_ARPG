#pragma once

#include <algorithm>
#include <filesystem>
#include <string>

namespace Nawia::Core::AssetPathUtils {

	[[nodiscard]] inline std::string normalizeSlashes(std::string path)
	{
		std::ranges::replace(path, '\\', '/');
		return path;
	}

	[[nodiscard]] inline std::string resolveModelPath(std::string model_path)
	{
		model_path = normalizeSlashes(std::move(model_path));
		if (model_path.empty() || model_path.rfind("assets/", 0) == 0)
			return model_path;

		if (std::filesystem::path(model_path).has_parent_path())
			return model_path;

		return "assets/models/" + model_path;
	}

} // namespace Nawia::Core::AssetPathUtils
