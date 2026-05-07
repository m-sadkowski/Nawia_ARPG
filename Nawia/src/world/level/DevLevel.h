#pragma once

#include <Level.h>
#include <raylib.h>
#include <string>
#include <vector>

namespace Nawia::World {

	/**
	 * @enum EditorMode
	 * @brief Określa aktualny stan UI kreatora leveli.
	 */
	enum class EditorMode {
		None,
		MainMenu,
		SpawnerType,
		SpawnerDetails,
		ChestDetails,
		NPCSelection,
		PropDetails,
		TeleportDetails,
		ItemSelection
	};

	/**
	 * @class DevLevel
	 * @brief Poziom developerski do ustawiania propow, spawnow i swiatla.
	 *
	 * Pozwala na szybkie poruszanie sie (Shift), a prawy klik otwiera
	 * zaawansowany kreator obiektow.
	 */
	class DevLevel : public Level {
	public:
		/** @brief Wczytuje mape developerska i ustawia gracza na bezpiecznym spawnie. */
		void onEnter(Core::Engine* engine) override;

		/** @brief Obsluguje tryb edycji albo wpisywanie danych w UI. */
		void handleInput(Core::Engine* engine) override;

		/** @brief Obsluguje szybkie poruszanie sie gracza. */
		void update(Core::Engine* engine, float dt) override;

		/** @brief Rysuje overlay developerski i menu kreatora. */
		void renderUI(Core::Engine* engine) override;

		/** @brief Zwraca nazwe poziomu developerskiego. */
		[[nodiscard]] std::string getName() const override { return "DevLevel"; }

		/** @brief DevLevel nie uzywa pliku spawnow. */
		[[nodiscard]] std::string getSpawnFilePath() const override { return ""; }

		/** @brief Zwraca pojedyncza lokacje robocza. */
		[[nodiscard]] std::vector<std::string> getLocations() const override {
			return {"Dev Sandbox"};
		}

		/** @brief Zwraca, czy aktywne menu blokuje input gracza. */
		[[nodiscard]] bool isTyping() const { return _current_mode != EditorMode::None; }

	private:
		struct PlacedObject {
			std::string category;
			std::string name;
			std::string type;
			Vector2 position;
			float spawn_radius = 0.0f;
			float trigger_radius = 0.0f;
			int count = 1;
			std::vector<int> loot_ids;
			std::string extra_string; // texture path, npc class, or target location
		};

		/** @brief Rysuje markery i zasiegi postawionych obiektow. */
		void renderPlacedObjects(Core::Engine* engine);

		/** @brief Usuwa najblizszy obiekt od pozycji myszy. */
		void deleteNearestObject(Core::Engine* engine);

		/** @brief Spawnuje wszystkie zaplanowane obiekty jako realne encje do testow. */
		void testLevel(Core::Engine* engine);

		/** @brief Obsluguje klawiature w UI kreatora. */
		void handleUIInput(Core::Engine* engine);

		/** @brief Obsluguje przesuwanie swiatla i otwieranie menu. */
		void handleEditingInput(Core::Engine* engine);

		/** @brief Rysuje dane aktualnego swiatla na ekranie. */
		void renderLightingOverlay(Core::Engine& engine) const;

		/** @brief Rysuje glowne menu kreatora. */
		void renderMainMenu(Core::Engine* engine);

		/** @brief Rysuje menu wyboru typu dla spawnera. */
		void renderSpawnerTypeMenu(Core::Engine* engine);

		/** @brief Rysuje menu szczegolow spawnera. */
		void renderSpawnerDetailsMenu(Core::Engine* engine);

		/** @brief Rysuje menu szczegolow skrzyni (loot). */
		void renderChestDetailsMenu(Core::Engine* engine);

		/** @brief Rysuje menu wyboru NPC. */
		void renderNPCSelectionMenu(Core::Engine* engine);

		/** @brief Rysuje menu szczegolow propa. */
		void renderPropDetailsMenu(Core::Engine* engine);

		/** @brief Rysuje menu szczegolow teleportu. */
		void renderTeleportDetailsMenu(Core::Engine* engine);

		/** @brief Rysuje menu wyboru przedmiotow. */
		void renderItemSelectionMenu(Core::Engine* engine);

		/** @brief Czysci stan kreatora. */
		void resetEditorState();

		EditorMode _current_mode = EditorMode::None;
		Vector2 _saved_world_position = { 0.0f, 0.0f };

		// Dane tymczasowe dla tworzonego obiektu
		std::string _temp_entity_type;
		std::string _temp_name;
		int _temp_count = 1;
		float _temp_spawn_radius = 5.0f;
		float _temp_trigger_radius = 10.0f;
		std::vector<int> _temp_loot_ids;
		std::string _temp_target_location;

		std::vector<PlacedObject> _placed_objects;

		// Obsluga pol tekstowych
		std::string _input_buffer;
		int _selected_field = 0;

		/** @brief Zapisuje obiekt do odpowiedniego pliku JSON i dodaje do listy sesji. */
		void saveObject(const std::string& category);

		/** @brief Nadpisuje pliki JSON aktualna lista obiektow. */
		void rewriteJsonFiles();
	};

} // namespace Nawia::World
