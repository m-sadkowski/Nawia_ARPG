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
#include <ResourceManager.h>
#include <SaveGameManager.h>
#include <Settings.h>
#include <UIHandler.h>

#include <memory>
#include <raylib.h>
#include <string>

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

		UI::UIHandler& getUIHandler() const { return *_ui_handler; }
		[[nodiscard]] bool isPaused() const { return _show_pause_menu || _game_state != GameState::Playing; }
		Map* getCurrentMap() const;
		Item::ItemDatabase& getItemDatabase() { return _item_database; }
		Game::DialogueManager& getDialogueManager() { return _dialogue_manager; }
		ResourceManager& getResourceManager() { return _resource_manager; }
		Item::Loottable& getLoottable() { return _loottable; }
		EntityManager& getEntityManager() const { return *_entity_manager; }
		std::shared_ptr<Entity::Player> getPlayer() const { return _player; }
		const GameCamera& getCamera() const { return _camera; }
		World::LevelManager& getLevelManager() const { return *_level_manager; }
		System::Renderer::LightingSystem& getLightingSystem() { return _lighting_system; }
		Game::QuestManager& getQuestManager() { return _quest_manager; }
		Audio::AudioManager& getAudioManager() { return _audio_manager; }
		Game::BossManager& getBossManager() { return _boss_manager; }
		const Game::BossManager& getBossManager() const { return _boss_manager; }
		Game::SaveGameManager& getSaveGameManager() { return _save_game_manager; }

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
		void collectPendingSpawns();
		void loadGameplaySounds();
		void applySettings(const Settings& new_settings);
		void createFreshPlayer(bool grant_starter_items);
		void startNewGame(const std::string& level_name, int default_slot);
		bool saveCurrentGame(int slot);
		/**
		 * @brief Wczytuje zapis z podanego slotu albo najnowszy zapis, gdy slot == 0.
		 */
		bool loadGameFromSlot(int slot);

		bool _is_running = false;
		GameState _game_state = GameState::Menu;
		bool _show_pause_menu = false;
		GameState _previous_state = GameState::Menu;
		std::string _pending_new_game_level; ///< Niepusta wartosc oznacza, ze trwa flow nowej gry.
		Settings _settings;

		Audio::AudioManager _audio_manager;
		System::Renderer::LightingSystem _lighting_system;
		ResourceManager _resource_manager;
		GameCamera _camera;
		std::unique_ptr<World::LevelManager> _level_manager;
		std::unique_ptr<EntityManager> _entity_manager;
		std::shared_ptr<Entity::Player> _player;
		std::unique_ptr<PlayerController> _controller;
		std::unique_ptr<UI::UIHandler> _ui_handler;
		Item::ItemDatabase _item_database;
		Item::Loottable _loottable;
		Game::DialogueManager _dialogue_manager;
		Game::QuestManager _quest_manager;
		Game::BossManager _boss_manager;
		Game::SaveGameManager _save_game_manager;
		Vector2 _last_hover_mouse_pos = {-10000.0f, -10000.0f};
		float _hover_update_timer = 0.0f;
	};

} // namespace Nawia::Core
