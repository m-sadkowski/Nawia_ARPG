#pragma once

#include <Level.h>

#include <json.hpp>
#include <raylib.h>

#include <filesystem>
#include <string>
#include <vector>

namespace Nawia::World {

	/**
	 * @enum EditorMode
	 * @brief Okresla aktualny ekran kreatora poziomu.
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
	 * @brief Poziom developerski do rozstawiania encji, testowania spawnow i strojenia swiatla.
	 *
	 * Prawy klik otwiera kreator obiektow w miejscu wskazanym kursorem, a szybki ruch
	 * gracza ulatwia inspekcje mapy.
	 */
	class DevLevel : public Level {
	public:
		/** @brief Wczytuje mape developerska i przygotowuje stan kreatora. */
		void onEnter(Core::Engine* engine) override;

		/** @brief Obsluguje input kreatora albo narzedzia edycji na mapie. */
		void handleInput(Core::Engine* engine) override;

		/** @brief Aktualizuje poziom i ruch gracza w trybie developerskim. */
		void update(Core::Engine* engine, float dt) override;

		/** @brief Rysuje overlay developerski i aktywny ekran kreatora. */
		void renderUI(Core::Engine* engine) override;

		/** @brief Zwraca nazwe poziomu developerskiego. */
		[[nodiscard]] std::string getName() const override { return "DevLevel"; }

		/** @brief DevLevel nie wczytuje standardowego pliku spawnow. */
		[[nodiscard]] std::string getSpawnFilePath() const override { return ""; }

		/** @brief Zwraca robocza lokacje edytora. */
		[[nodiscard]] std::vector<std::string> getLocations() const override { return {"Dev Sandbox"}; }

		/** @brief Zwraca, czy aktywne menu kreatora blokuje input gracza. */
		[[nodiscard]] bool isTyping() const { return _current_mode != EditorMode::None; }

	private:
		struct PlacedObject {
			std::string category;
			std::string name;
			std::string type;
			Vector2 position = {0.0f, 0.0f};
			float spawn_radius = 0.0f;
			float trigger_radius = 0.0f;
			int count = 1;
			std::vector<int> loot_ids;
			std::string extra_value;
		};

		/** @brief Wczytuje zapisane obiekty z roboczych plikow JSON. */
		void loadPlacedObjects();

		/** @brief Wczytuje jedna kategorie obiektow z pliku JSON. */
		void loadPlacedObjectsFromFile(const std::string& category, const std::filesystem::path& path);

		/** @brief Zamienia wpis JSON na obiekt sesji edytora. */
		[[nodiscard]] PlacedObject parsePlacedObject(const std::string& category, const nlohmann::json& data) const;

		/** @brief Buduje JSON dla obiektu zapisywanego przez kreator. */
		[[nodiscard]] nlohmann::json serializePlacedObject(const PlacedObject& object) const;

		/** @brief Zwraca sciezke do roboczego pliku kategorii. */
		[[nodiscard]] std::filesystem::path getCategoryFilePath(const std::string& category) const;

		/** @brief Rysuje markery i zasiegi postawionych obiektow. */
		void renderPlacedObjects(Core::Engine* engine);

		/** @brief Usuwa obiekt wskazany kursorem albo najblizszy punktowi klikniecia. */
		void deleteNearestObject(Core::Engine* engine);

		/** @brief Spawnuje wszystkie zaplanowane obiekty jako realne encje do testow. */
		void testLevel(Core::Engine* engine);

		/** @brief Obsluguje klawiature w UI kreatora. */
		void handleUIInput(Core::Engine* engine);

		/** @brief Obsluguje przesuwanie swiatla i otwieranie kreatora. */
		void handleEditingInput(Core::Engine* engine);

		/** @brief Rysuje instrukcje i status narzedzi developerskich. */
		void renderLightingOverlay(Core::Engine& engine) const;

		/** @brief Rysuje glowne menu kreatora. */
		void renderMainMenu(Core::Engine* engine);

		/** @brief Rysuje menu wyboru typu przeciwnika dla spawnera. */
		void renderSpawnerTypeMenu(Core::Engine* engine);

		/** @brief Rysuje formularz szczegolow spawnera. */
		void renderSpawnerDetailsMenu(Core::Engine* engine);

		/** @brief Rysuje formularz skrzyni i wyboru lootu. */
		void renderChestDetailsMenu(Core::Engine* engine);

		/** @brief Rysuje menu wyboru NPC. */
		void renderNPCSelectionMenu(Core::Engine* engine);

		/** @brief Rysuje formularz obiektu statycznego. */
		void renderPropDetailsMenu(Core::Engine* engine);

		/** @brief Rysuje formularz teleportu. */
		void renderTeleportDetailsMenu(Core::Engine* engine);

		/** @brief Rysuje wybor przedmiotow z bazy itemow. */
		void renderItemSelectionMenu(Core::Engine* engine);

		/** @brief Czysci stan tworzonego obiektu i formularzy. */
		void resetEditorState();

		/** @brief Dodaje obiekt do sesji i zapisuje pliki JSON. */
		void saveObject(const std::string& category);

		/** @brief Nadpisuje robocze pliki JSON aktualna lista obiektow. */
		void rewriteJsonFiles();

		EditorMode _current_mode = EditorMode::None;
		Vector2 _saved_world_position = {0.0f, 0.0f};

		std::string _temp_entity_type;
		std::string _temp_name;
		int _temp_count = 1;
		float _temp_spawn_radius = 5.0f;
		float _temp_trigger_radius = 15.0f;
		std::vector<int> _temp_loot_ids;
		std::string _temp_extra_value;

		std::string _count_buffer = "1";
		std::string _spawn_radius_buffer = "5.0";
		std::string _trigger_radius_buffer = "15.0";
		std::string _texture_path_buffer = "assets/textures/chest.png";
		int _selected_field = 0;

		std::vector<PlacedObject> _placed_objects;
	};

} // namespace Nawia::World
