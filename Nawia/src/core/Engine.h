#pragma once

#include <AudioManager.h>
#include <Camera.h>
#include <Constants.h>
#include <DialogueManager.h>
#include <EntityManager.h>
#include <ItemDatabase.h>
#include <LevelManager.h>
#include <LightingSystem.h>
#include <Loottable.h>
#include <Player.h>
#include <QuestManager.h>
#include <BossManager.h>
#include <CombatEventBus.h>
#include <CombatTelemetryServer.h>
#include <ResourceManager.h>
#include <SaveGameManager.h>
#include <Settings.h>
#include <CustomCursor.h>
#include <UIHandler.h>

#include <AssetLoadManifest.h>

#include <json.hpp>

#include <memory>
#include <raylib.h>
#include <string>
#include <vector>

namespace Nawia::Core {

	class Map;
	class PlayerController;

	/**
	 * @class Engine
	 * @brief Glowny wlasciciel petli gry i najwazniejszych systemow.
	 *
	 * Systemy z cyklem zycia zaleznym od silnika sa trzymane przez `unique_ptr`
	 * albo jako pola wartosciowe. Gracz jest `shared_ptr`, bo wspoldziela go
	 * Engine, EntityManager i kontroler.
	 */
	class Engine {
	public:
		/**
		 * @enum GameState
		 * @brief Ekran albo tryb, w ktorym aktualnie znajduje sie gra.
		 */
		enum class GameState {
			Loading,
			Menu,
			SettingsMenu,
			LevelSelect,
			SaveSlotSelect,
			Playing,
			GameOver
		};

		Engine();
		~Engine();

		/** @brief Uruchamia petle gry. */
		void run();

		/** @brief Sprawdza, czy okno i petla gry nadal dzialaja. */
		[[nodiscard]] bool isRunning() const;

		/** @brief Zwraca encje pod kursorem albo nullptr. */
		[[nodiscard]] std::shared_ptr<Entity::Entity> getEntityAt(float screen_x, float screen_y) const;

		/** @brief Dodaje encje utworzona przez gameplay, np. efekt umiejetnosci. */
		void spawnEntity(std::shared_ptr<Entity::Entity> new_entity) const;

		UI::UIHandler& getUIHandler() const;
		[[nodiscard]] bool isPaused() const { return _show_pause_menu || _game_state != GameState::Playing; }
		Map* getCurrentMap() const;
		Item::ItemDatabase& getItemDatabase() { return _item_database; }
		Game::DialogueManager& getDialogueManager() { return _dialogue_manager; }
		ResourceManager& getResourceManager() { return _resource_manager; }
		Item::Loottable& getLoottable() { return _loottable; }
		EntityManager& getEntityManager() const { return *_entity_manager; }
		std::shared_ptr<Entity::Player> getPlayer() const { return _player; }
		GameCamera& getCamera() { return _camera; }
		const GameCamera& getCamera() const { return _camera; }
		World::LevelManager& getLevelManager() const { return *_level_manager; }
		System::Renderer::LightingSystem& getLightingSystem() { return _lighting_system; }
		Game::QuestManager& getQuestManager() { return _quest_manager; }
		Audio::AudioManager& getAudioManager() { return _audio_manager; }
		Game::CombatEventBus& getCombatEventBus() { return _combat_event_bus; }
		const Game::CombatEventBus& getCombatEventBus() const { return _combat_event_bus; }
		Game::BossManager& getBossManager() { return _boss_manager; }
		const Game::BossManager& getBossManager() const { return _boss_manager; }
		Game::SaveGameManager& getSaveGameManager() { return _save_game_manager; }
		void setGameplayCameraZoom(float zoom_factor) {
			_gameplay_camera_zoom = zoom_factor;
			_current_camera_zoom = zoom_factor;
			_camera.resetZoom(zoom_factor);
		}
		[[nodiscard]] float getGameplayCameraZoom() const { return _gameplay_camera_zoom; }
		void notifyStoryEvent(const std::string& event_id, Vector2 world_position);
		void cancelPlayerAction();
		void requestReturnToMainMenu(std::string notification = "");

		/**
		 * @brief Zapisuje stan gry do aktywnego slotu, jesli jakikolwiek jest ustawiony.
		 *
		 * Wywolywane np. przez checkpointy. Bez aktywnego slotu (np. po starcie
		 * gry uruchomionej z trybu deweloperskiego) operacja jest pomijana.
		 */
		bool saveGameToActiveSlot();
	private:
		void update(float delta_time);
		void render() const;
		void handleInput();
		void handleMenuInput();
		void handleGameOverInput();
		void handleSettingsInput();
		void handleLevelSelectInput();
		void handleSaveSlotSelectInput();
		void handlePlayingInput();
		void renderWorld() const;
		void renderGameplay() const;
		void renderGameplayVignetteOverlay() const;
		[[nodiscard]] bool isLevelInteractionOnly() const;
		[[nodiscard]] bool isLevelBlockingControl() const;
		[[nodiscard]] float getLevelCameraZoomMultiplier() const;
		[[nodiscard]] float getLevelCameraTargetHeightMultiplier() const;
		void collectPendingSpawns();
		void loadGameplaySounds();
		void applySettings(const Settings& new_settings);
		void createFreshPlayer();
		void startNewGame(const std::string& level_name, int default_slot);
		bool saveCurrentGame(int slot);
		/**
		 * @brief Wczytuje zapis z podanego slotu albo najnowszy zapis, gdy slot == 0.
		 */
		bool loadGameFromSlot(int slot);

		void processLoading();
		void finishStartupLoading();
		void finishLevelLoading();
		void queueLevelLoad(
			const std::string& level_name,
			const std::string& initial_location,
			bool is_new_game,
			int default_slot
		);
		enum class LoadingKind { None, Startup, Level };

		LoadingKind _loading_kind = LoadingKind::Startup; ///< Rodzaj ladowania aktualnie obslugiwany przez ekran.
		AssetLoadManifest _loading_manifest;              ///< Uporzadkowana lista assetow do preloadu.
		size_t _loading_asset_index = 0;
		float _loading_progress = 0.0f;
		std::string _loading_status; ///< Krotki status wyswietlany przez UI ladowania.
		std::string _loading_title;  ///< Tytul ekranu ladowania.
		std::string _pending_level_name;       ///< Zakolejkowany poziom docelowy do wczytania.
		std::string _pending_initial_location; ///< Opcjonalna lokacja startowa w zakolejkowanym poziomie.
		bool _pending_is_new_game = false;
		int _pending_new_game_slot = 0;
		bool _has_pending_save = false; ///< Prawda, gdy wczytany zapis czeka na systemy poziomu.
		nlohmann::json _pending_save_state;
		int _pending_save_slot = 0;

		bool _is_running = false;
		GameState _game_state = GameState::Loading; ///< Autorytatywny tryb ekranu/gry.
		bool _show_pause_menu = false;
		bool _pending_return_to_main_menu = false;
		std::string _pending_return_notification;
		GameState _previous_state = GameState::Menu; ///< Uzywany przy powrocie ze stanow modalnych menu.
		std::string _pending_new_game_level; ///< Niepusta wartosc oznacza, ze trwa flow nowej gry.
		Settings _settings; ///< Robocza kopia stosowana do renderera, audio i okna.

		// Serwisy posiadane przez Engine. Centralne posiadanie pozwala kodowi
		// poziomow i encji uzywac wskaznikow bez przejmowania cyklu zycia.
		Audio::AudioManager _audio_manager;
		System::Renderer::LightingSystem _lighting_system;
		ResourceManager _resource_manager;
		GameCamera _camera;
		float _gameplay_camera_zoom = 0.75f; ///< Bazowy zoom wczytany z aktualnej lokacji.
		float _current_camera_zoom = 0.75f;  ///< Faktyczny zoom po mnoznikach specyficznych dla poziomu.
		float _current_camera_target_height_multiplier = 1.0f; ///< Tymczasowa korekta pionowego kadrowania.
		std::unique_ptr<World::LevelManager> _level_manager;
		std::unique_ptr<EntityManager> _entity_manager;
		std::shared_ptr<Entity::Player> _player;
		std::unique_ptr<PlayerController> _controller;
		std::unique_ptr<UI::UIHandler> _ui_handler;
		Item::ItemDatabase _item_database;
		Item::Loottable _loottable;
		Game::DialogueManager _dialogue_manager;
		Game::QuestManager _quest_manager;
		Game::CombatEventBus _combat_event_bus;
		Game::CombatEventBus::SubscriptionId _combat_telemetry_subscription_id = 0;
		Game::CombatTelemetryServer _combat_telemetry_server;
		Game::BossManager _boss_manager;
		Game::SaveGameManager _save_game_manager;
		mutable UI::CustomCursor _custom_cursor; ///< Customowy kursor gry w stylu slowianskim.
		Vector2 _last_hover_mouse_pos = {-10000.0f, -10000.0f}; ///< Ostatnia pozycja myszy dla raycastow hover.
		float _hover_update_timer = 0.0f; ///< Ogranicza kosztowne testy hover 3D.
	};

} // namespace Nawia::Core
