#pragma once
#include "Camera.h"
#include "Constants.h"
#include "EntityManager.h"
#include "Map.h"
#include "ResourceManager.h"
#include "Settings.h"

#include <Player.h>
#include <UIHandler.h>
#include <ItemDatabase.h>

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
			Playing         ///< Gameplay
		};

		Engine();
		~Engine();

		void run();
		[[nodiscard]] bool isRunning() const;

		[[nodiscard]] std::shared_ptr<Entity::Entity> getEntityAt(float screen_x, float screen_y) const;
		void spawnEntity(const std::shared_ptr<Entity::Entity>& new_entity) const;

		UI::UIHandler& getUIHandler() const { return *_ui_handler; }
	private:
		void update(float delta_time);
		void render() const;
		void handleInput();
		
		/// Apply new settings (resolution change, etc.)
		void applySettings(const Settings& new_settings);

		bool _is_running;
		GameState _game_state;
		bool _show_pause_menu = false;  ///< ESC menu overlay during gameplay
		GameState _previous_state = GameState::Menu;  ///< State before opening settings
		Settings _settings;

		ResourceManager _resource_manager;
		Camera _camera;
		std::unique_ptr<Map> _map;
		std::unique_ptr<EntityManager> _entity_manager;
		std::shared_ptr<Entity::Player> _player;
		std::unique_ptr<PlayerController> _controller;
		std::unique_ptr<UI::UIHandler> _ui_handler;
		Item::ItemDatabase _item_database;
	};

} // namespace Nawia::Core