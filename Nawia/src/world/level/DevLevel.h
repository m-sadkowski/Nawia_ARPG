#pragma once

#include <Level.h>

#include <raylib.h>

namespace Nawia::World {

	/**
	 * @class DevLevel
	 * @brief Poziom developerski do ustawiania propow i swiatla.
	 *
	 * Prawy klik zapisuje pozycje propa, a kontrolki swiatla pozwalaja
	 * dostroic i zapisac oswietlenie mapy.
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

		/** @brief Zwraca, czy aktywne pole tekstowe blokuje input gracza. */
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

		/** @brief Dopisuje nazwany obiekt z pozycja swiata do pliku JSON. */
		void saveObjectToJson(const std::string& object_name, float world_x, float world_z);
	};

} // namespace Nawia::World
