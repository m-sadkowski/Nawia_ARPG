#pragma once

#include <json.hpp>

namespace Nawia::Core { class Engine; }

namespace Nawia::Game {

	/**
	 * @brief Sprawdza warunki fabularne zapisane w JSON.
	 */
	[[nodiscard]] bool areStoryConditionsMet(const nlohmann::json& data, Core::Engine* engine);

	/**
	 * @brief Sprawdza warunki aktywacji encji, laczac `conditions`, `spawn_if`
	 * oraz negatywne `hide_if`.
	 */
	[[nodiscard]] bool areEntityConditionsMet(const nlohmann::json& entity_data, Core::Engine* engine);

} // namespace Nawia::Game
