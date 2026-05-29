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
		SpawnerType,
		SpawnerDetails,
		ChestDetails,
		NPCSelection,
		PropDetails,
		TeleportDetails,
		BossTriggerDetails,
		ItemSelection,
		KeySelection,
		ConfirmOverwrite
	};

	/**
	 * @enum EditorTextField
	 * @brief Identyfikuje aktywne pole tekstowe stalego HUD-u albo okna modalnego.
	 */
	enum class EditorTextField {
		None,
		MapScale,
		OffsetX,
		OffsetY,
		OffsetZ,
		RotationX,
		RotationY,
		RotationZ,
		NavmeshMinHeight,
		LocationName,
		LocationFile,
		SpawnerCount,
		SpawnerRadius,
		TriggerRadius,
		ObjectName,
		PropModel,
		TeleportTarget,
		BossTriggerWidth,
		BossTriggerHeight,
		ChestKeyId
	};

	/**
	 * @class DevLevel
	 * @brief Poziom developerski do rozstawiania encji, testowania spawnow i strojenia swiatla.
	 *
	 * Staly HUD kreatora pozwala wybrac model mapy, zapisac lokacje i dodawac
	 * obiekty w miejscu, w ktorym stoi gracz.
	 */
	class DevLevel : public Level {
	public:
		/** @brief Wczytuje mape developerska i przygotowuje stan kreatora. */
		void onEnter(Core::Engine* engine) override;

		/** @brief Sprzata tryb developerski po wyjsciu z poziomu. */
		void onExit(Core::Engine* engine) override;

		/** @brief Obsluguje input kreatora albo narzedzia edycji na mapie. */
		void handleInput(Core::Engine* engine) override;

		/** @brief Aktualizuje poziom i ruch gracza w trybie developerskim. */
		void update(Core::Engine* engine, float dt) override;

		/** @brief Rysuje overlay developerski i aktywny ekran kreatora. */
		void renderUI(Core::Engine* engine) override;

		/** @brief Zwraca nazwe kreatora widoczna w menu wyboru poziomu. */
		[[nodiscard]] std::string getName() const override { return "Kreator leveli"; }

		/** @brief Zwraca robocza lokacje edytora. */
		[[nodiscard]] std::vector<std::string> getLocations() const override;

		/** @brief Kreator leveli nie uczestniczy w systemie zapisu i wczytywania. */
		[[nodiscard]] bool allowsSaves() const override { return false; }

		/** @brief Zwraca, czy aktywne menu kreatora blokuje input gracza. */
		[[nodiscard]] bool isTyping() const;

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
			bool locked = false;
			int key_id = -1;
			std::string extra_value;
			nlohmann::json raw_data = nlohmann::json::object();
		};

		struct LocationOption {
			std::string display_name;
			std::string location_name;
			std::filesystem::path path;
			std::filesystem::path objects_path;
		};

		/** @brief Wczytuje liste modeli map z assets/maps. */
		void loadAvailableMapModels();

		/** @brief Wczytuje liste lokacji z JSON-ow dostepnych w repozytorium. */
		void loadAvailableLocations();

		/** @brief Wczytuje identyfikatory bossow z bosses.json. */
		void loadAvailableBossIds();

		/** @brief Ustawia pusta nowa lokacje z placeholderem mapy. */
		void initializeNewLocation(Core::Engine* engine);

		/** @brief Wczytuje wybrana lokacje z listy albo przygotowuje nowa. */
		void loadLocationFromOption(Core::Engine* engine, int option_index);

		/** @brief Wczytuje obiekty z pliku JSON lokacji. */
		void loadPlacedObjectsFromFile(const std::filesystem::path& path, const std::string& location_filter = "");

		/** @brief Zamienia wpis JSON na obiekt sesji edytora. */
		[[nodiscard]] PlacedObject parsePlacedObject(const nlohmann::json& data) const;

		/** @brief Buduje JSON dla obiektu zapisywanego przez kreator. */
		[[nodiscard]] nlohmann::json serializePlacedObject(const PlacedObject& object) const;

		/** @brief Przenosi aktualny stan ustawien do buforow pol tekstowych. */
		void applyLocationStateToBuffers();

		/** @brief Parsuje bufory pol tekstowych do stanu edytora. */
		bool syncLocationStateFromBuffers();

		/** @brief Przeladowuje model mapy i przebudowuje navmesh z ustawien edytora. */
		void reloadMapFromEditor(Core::Engine* engine, bool move_player_to_spawn);

		/** @brief Ustawia spawn aktualnej lokacji na pozycji gracza. */
		void setSpawnFromPlayer(Core::Engine* engine);

		/** @brief Zwraca pozycje gracza w plaszczyznie XZ. */
		[[nodiscard]] Vector2 getPlayerPosition(Core::Engine* engine) const;

		/** @brief Tworzy docelowa sciezke pliku lokacji z bufora. */
		[[nodiscard]] std::filesystem::path getLocationFilePathFromBuffer() const;

		/** @brief Tworzy sciezke pliku obiektow objects_*.json dla lokacji. */
		[[nodiscard]] std::filesystem::path getObjectsFilePath(const std::filesystem::path& location_path) const;

		/** @brief Rozpoczyna zapis, pytajac o nadpisanie gdy pliki istnieja. */
		void requestSaveLocation();

		/** @brief Zapisuje plik lokacji oraz odpowiadajacy mu objects_*.json. */
		void saveLocationFiles(const std::filesystem::path& location_path, const std::filesystem::path& objects_path);

		/** @brief Rysuje markery i zasiegi postawionych obiektow. */
		void renderPlacedObjects(Core::Engine* engine);

		/** @brief Usuwa obiekt wskazany kursorem albo najblizszy punktowi klikniecia. */
		void deleteNearestObject(Core::Engine* engine);

		/** @brief Spawnuje wszystkie zaplanowane obiekty jako realne encje do testow. */
		void testLevel(Core::Engine* engine);

		/** @brief Konczy test i usuwa testowo zespawnowane encje. */
		void stopTestLevel(Core::Engine* engine);

		/** @brief Obsluguje klawiature w UI kreatora. */
		void handleUIInput(Core::Engine* engine);

		/** @brief Obsluguje przesuwanie i zapis swiatla. */
		void handleEditingInput(Core::Engine* engine);

		/** @brief Rysuje instrukcje i status narzedzi developerskich. */
		void renderLightingOverlay(Core::Engine& engine);

		/** @brief Rysuje plaszczyzne odcinajaca niska wode z navmesha. */
		void renderWaterCutoffPlane(Core::Engine& engine) const;

		/** @brief Rysuje staly HUD kreatora. */
		void renderEditorHud(Core::Engine* engine);

		/** @brief Rysuje lewa kolumne ustawien mapy. */
		void renderMapPanel(Core::Engine* engine);

		/** @brief Rysuje srodkowa kolumne wyboru lokacji i zapisu. */
		void renderLocationPanel(Core::Engine* engine);

		/** @brief Rysuje prawa kolumne dodawania obiektow. */
		void renderObjectPanel(Core::Engine* engine);

		/** @brief Rysuje prosty przeglad animacji gracza. */
		void renderPlayerAnimationPanel(Core::Engine* engine, int x, int y);
		void playSelectedPlayerAnimation(Core::Engine* engine);

		/** @brief Rysuje rozwiniete listy nad wszystkimi panelami. */
		void renderDropdownOverlays(Core::Engine* engine);

		/** @brief Rysuje pytanie o potwierdzenie nadpisania plikow. */
		void renderConfirmOverwriteDialog();

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

		/** @brief Rysuje formularz triggera bossa. */
		void renderBossTriggerDetailsMenu(Core::Engine* engine);

		/** @brief Rysuje wybor przedmiotow z bazy itemow. */
		void renderItemSelectionMenu(Core::Engine* engine);

		/** @brief Rysuje wybor przedmiotu uzywanego jako klucz do skrzyni. */
		void renderKeySelectionMenu(Core::Engine* engine);

		/** @brief Czysci stan tworzonego obiektu i formularzy. */
		void resetEditorState();

		/** @brief Ustawia miejsce dodawania obiektu na aktualna pozycje gracza. */
		void prepareObjectPlacementAtPlayer(Core::Engine* engine);

		/** @brief Dodaje obiekt do sesji; zapis nastepuje dopiero przy zapisie lokacji. */
		void saveObject(const std::string& category);

		/** @brief Sprawdza, czy kursor jest nad panelami edytora. */
		[[nodiscard]] bool isMouseOverEditorUI() const;

		/** @brief Zwraca, czy aktywne jest dowolne pole tekstowe. */
		[[nodiscard]] bool isTextFieldActive() const { return _active_text_field != EditorTextField::None; }

		/** @brief Zwraca, czy otwarto ktorys dropdown. */
		[[nodiscard]] bool isAnyDropdownOpen() const {
			return _map_dropdown_open || _location_dropdown_open || _teleport_target_dropdown_open || _boss_dropdown_open;
		}

		EditorMode _current_mode = EditorMode::None;
		EditorTextField _active_text_field = EditorTextField::None;
		Vector2 _saved_world_position = {0.0f, 0.0f};

		std::string _temp_entity_type;
		std::string _temp_name;
		int _temp_count = 1;
		float _temp_spawn_radius = 5.0f;
		float _temp_trigger_radius = 15.0f;
		std::vector<int> _temp_loot_ids;
		bool _temp_chest_locked = false;
		int _temp_key_id = -1;
		std::string _temp_extra_value;

		std::string _count_buffer = "1";
		std::string _spawn_radius_buffer = "5.0";
		std::string _trigger_radius_buffer = "15.0";
		std::string _prop_model_path_buffer;
		std::string _boss_width_buffer = "10.0";
		std::string _boss_height_buffer = "4.0";
		std::string _key_id_buffer = "-1";
		float _navmesh_min_walkable_height = 0.0f;

		std::vector<std::string> _map_model_options;
		std::vector<LocationOption> _location_options;
		std::vector<std::string> _boss_id_options;
		int _selected_map_model_index = 0;
		int _selected_location_index = 0;
		int _selected_teleport_target_index = 0;
		int _selected_boss_index = 0;
		int _selected_player_animation_index = 0;
		bool _map_dropdown_open = false;
		bool _location_dropdown_open = false;
		bool _teleport_target_dropdown_open = false;
		bool _boss_dropdown_open = false;

		std::string _active_location_name = "Nowa lokacja";
		std::string _location_name_buffer = "Nowa lokacja";
		std::string _location_file_buffer = "nowa_lokacja.json";

		std::string _map_scale_buffer = "1.0";
		std::string _offset_x_buffer = "0.0";
		std::string _offset_y_buffer = "0.0";
		std::string _offset_z_buffer = "0.0";
		std::string _rotation_x_buffer = "0.0";
		std::string _rotation_y_buffer = "0.0";
		std::string _rotation_z_buffer = "0.0";
		std::string _navmesh_height_buffer = "0.0";

		std::string _active_map_model = "placeholder";
		float _active_map_scale = 1.0f;
		Vector3 _active_map_offset = {0.0f, 0.0f, 0.0f};
		Vector3 _active_map_rotation = {0.0f, 0.0f, 0.0f};
		float _camera_zoom = 0.75f;
		Vector2 _player_spawn = {0.0f, 0.0f};
		bool _has_player_spawn = true;
		bool _has_unsaved_changes = false;
		bool _is_testing_level = false;

		std::filesystem::path _pending_location_save_path;
		std::filesystem::path _pending_objects_save_path;
		std::string _status_message;

		std::vector<PlacedObject> _placed_objects;
	};

} // namespace Nawia::World
