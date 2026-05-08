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
#include <ResourceManager.h>
#include <Settings.h>
#include <UIHandler.h>

#include <memory>
#include <raylib.h>

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

	private:
		void update(float delta_time);
		void render() const;
		void handleInput();
		void handleMenuInput();
		void handleGameOverInput();
		void handleSettingsInput();
		void handleLevelSelectInput();
		void handlePlayingInput();
		void renderWorld() const;
		void renderGameplay() const;
		void collectPendingSpawns();
		void loadGameplaySounds();
		void applySettings(const Settings& new_settings);

		bool _is_running = false;
		GameState _game_state = GameState::Menu;
		bool _show_pause_menu = false;
		GameState _previous_state = GameState::Menu;
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
	};

} // namespace Nawia::Core
