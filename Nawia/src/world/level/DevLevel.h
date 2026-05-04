#pragma once

#include "Level.h"

#include <raylib.h>

namespace Nawia::World {

	/**
	 * @class DevLevel
	 * @brief Development level for placing props and tuning lighting.
	 *
	 * Right-click on the map to open a text prompt, type a prop name,
	 * and press Enter to save its world coordinates to
	 * `assets/data/static_objects_dev.json`.
	 *
	 * The level also exposes simple controls for moving the primary light
	 * and saving the whole lighting setup to `assets/maps/forest_lighting.json`.
	 */
	class DevLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;

		void handleInput(Core::Engine* engine) override;
		void update(Core::Engine* engine, float dt) override;
		void renderUI(Core::Engine* engine) override;

		[[nodiscard]] std::string getName() const override { return "DevLevel"; }
		[[nodiscard]] std::string getSpawnFilePath() const override { return ""; }
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Dev Sandbox"};
		}

		/** @brief Returns true when the text input prompt is active (blocks player input). */
		[[nodiscard]] bool isTyping() const { return _is_typing; }

	private:
		void handleTypingInput();
		void handleEditingInput(Core::Engine* engine);
		void renderLightingOverlay(Core::Engine& engine) const;
		void renderPropPlacementPrompt() const;
		void clearTypingState();

		bool _is_typing = false;
		std::string _input_text;
		Vector2 _saved_world_position = { 0.0f, 0.0f };

		/** @brief Appends a named object entry with world coordinates to the JSON file. */
		void saveObjectToJson(const std::string& object_name, float world_x, float world_z);
	};

} // namespace Nawia::World
