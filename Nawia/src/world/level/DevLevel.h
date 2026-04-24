#pragma once

#include "Level.h"

#include <raylib.h>

namespace Nawia::World {

	/**
	 * @class DevLevel
	 * @brief Development/debug level for placing static objects.
	 *
	 * Right-click on the map to open a text prompt; type a prop name
	 * and press Enter to save its world coordinates to
	 * "static_objects_dev.json". Uses the "demo_map/inferno.glb" map.
	 */
	class DevLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;

		void handleInput(Core::Engine* engine) override;
		void update(Core::Engine* engine, float dt) override;
		void renderUI(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "DevLevel"; }
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Dev Sandbox"};
		}

		/** @brief Returns true when the text input prompt is active (blocks player input). */
		[[nodiscard]] bool isTyping() const { return _is_typing; }

	private:
		bool _is_typing = false;
		std::string _input_text;
		Vector2 _saved_iso_pos = {0, 0};

		/** @brief Appends a named object entry with world coordinates to the JSON file. */
		void saveObjectToJson(const std::string& name, float x, float y);
	};

} // namespace Nawia::World
