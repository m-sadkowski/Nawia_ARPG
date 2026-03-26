#pragma once
#include "Camera.h"
#include "Constants.h"
#include "EntityManager.h"
#include "LevelManager.h"
#include "ResourceManager.h"
#include "Settings.h"

#include <Player.h>
#include <UIHandler.h>
#include <ItemDatabase.h>
#include <Loottable.h>
#include <DialogueManager.h>

#include <raylib.h>

namespace Nawia::Core {

	class PlayerController;

	/**
	 * @class Engine
	 * @brief Main game engine managing game loop, state, and subsystems.
	 */
	class Engine {
	public:
		/// Game states
		enum class GameState {
			Menu,           ///< Main menu
			SettingsMenu,   ///< Settings menu overlay
			LevelSelect,    ///< Level Selection overlay
			Playing         ///< Gameplay
		};

		Engine();
		~Engine();

		void run();
		[[nodiscard]] bool isRunning() const;

		[[nodiscard]] std::shared_ptr<Entity::Entity> getEntityAt(float screen_x, float screen_y) const;
		void spawnEntity(const std::shared_ptr<Entity::Entity>& new_entity) const;

		UI::UIHandler& getUIHandler() const { return *_ui_handler; }
		Map* getCurrentMap() const {
			if (_level_manager && _level_manager->getCurrentLevel()) {
				return _level_manager->getCurrentLevel()->getMap();
			}
			return nullptr;
		}
		Item::ItemDatabase& getItemDatabase() { return _item_database; }
		Game::DialogueManager& getDialogueManager() { return _dialogue_manager; }
		ResourceManager& getResourceManager() { return _resource_manager; }
		Item::Loottable& getLoottable() { return _loottable; }
		EntityManager& getEntityManager() const { return *_entity_manager; }
		std::shared_ptr<Entity::Player> getPlayer() const { return _player; }
		const GameCamera& getCamera() const { return _camera; }
	private:
		void update(float delta_time);
		void render() const;
		void handleInput();
		
		/// Apply new settings (resolution change, etc.)
		void applySettings(const Settings& new_settings);

		bool _is_running;
		GameState _game_state;
		bool _show_pause_menu = false;
		GameState _previous_state = GameState::Menu;
		Settings _settings;

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
	};

} // namespace Nawia::Core