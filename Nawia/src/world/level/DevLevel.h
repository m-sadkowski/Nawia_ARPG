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
		/** @brief Wczytuje mape developerska i ustawia gracza na bezpiecznym spawnie. */
		void onEnter(Core::Engine* engine) override;

		/** @brief Obsluguje tryb edycji albo wpisywanie nazwy propa. */
		void handleInput(Core::Engine* engine) override;

		/** @brief Zachowuje wspolny interfejs poziomu; logika jest obslugiwana inputem. */
		void update(Core::Engine* engine, float dt) override;

		/** @brief Rysuje overlay developerski i pole nazwy propa. */
		void renderUI(Core::Engine* engine) override;

		/** @brief Zwraca nazwe poziomu developerskiego. */
		[[nodiscard]] std::string getName() const override { return "DevLevel"; }

		/** @brief DevLevel nie uzywa pliku spawnow. */
		[[nodiscard]] std::string getSpawnFilePath() const override { return ""; }

		/** @brief Zwraca pojedyncza lokacje robocza. */
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Dev Sandbox"};
		}

		/** @brief Zwraca, czy aktywne pole tekstowe blokuje input gracza. */
		[[nodiscard]] bool isTyping() const { return _is_typing; }

	private:
		/** @brief Przetwarza wpisywana nazwe propa i zapisuje ja po Enterze. */
		void handleTypingInput();

		/** @brief Obsluguje przesuwanie swiatla i zapisywanie pozycji propa. */
		void handleEditingInput(Core::Engine* engine);

		/** @brief Rysuje dane aktualnego swiatla na ekranie. */
		void renderLightingOverlay(Core::Engine& engine) const;

		/** @brief Rysuje modal wpisywania nazwy zapisywanego propa. */
		void renderPropPlacementPrompt() const;

		/** @brief Czysci stan wpisywania nazwy propa. */
		void clearTypingState();

		bool _is_typing = false;
		std::string _input_text;
		Vector2 _saved_world_position = { 0.0f, 0.0f };

		/** @brief Dopisuje nazwany obiekt z pozycja swiata do pliku JSON. */
		void saveObjectToJson(const std::string& object_name, float world_x, float world_z);
	};

} // namespace Nawia::World
