#include "Engine.h"
#include "GlobalScaling.h"
#include "Logger.h"
#include "MathUtils.h"
#include "PlayerController.h"

#include <FireballAbility.h>
#include <SwordSlashAbility.h>
#include <DemoLevel.h>
#include <DevLevel.h>
#include <LevelManager.h>

namespace Nawia::Core {

	Engine::Engine() : _is_running(false), _controller(nullptr), _game_state(GameState::Menu) 
	{
		SetTraceLogLevel(LOG_ERROR);
		InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Nawia");
		SetExitKey(0);  // Disable ESC = close window (we handle ESC manually)
		SetTargetFPS(0);
		
		// Load settings from file (if exists)
		if (_settings.load()) {
		    // Apply saved resolution
		    SetWindowSize(_settings.resolution.width, _settings.resolution.height);
		}
		
		// Initialize UI scaling system with saved manual scale
		GlobalScaling::setManualScale(_settings.ui_scale);

		// init item database
		_item_database.loadDatabase("../assets/data/items.json", _resource_manager);
		Logger::debugLog("Zaladowano baze danych przedmiotow");

		// init loottables
		_loottable.loadLootTables("../assets/data/loottables.json", _item_database);

		// initialize player
		Vector2 player_spawn_pos = {10.0f, -6.5f};
		_player = Entity::PlayerBuilder(this).setPosition(player_spawn_pos).build();

		// initialize spells
		const auto sword_slash_tex = _resource_manager.getTexture("../assets/textures/sword_slash.png");
		const auto sword_slash_icon = _resource_manager.getTexture("../assets/textures/icons/sword_slash_icon.png");
		_player->addAbility(std::make_shared<Entity::SwordSlashAbility>(sword_slash_tex, sword_slash_icon));
		
		const auto fireball_tex = _resource_manager.getTexture("../assets/textures/fireball.png");
		const auto fireball_hit_tex = _resource_manager.getTexture("../assets/textures/fireball_hit.png");
		const auto fireball_icon = _resource_manager.getTexture("../assets/textures/icons/fireball_icon.png");
		_player->addAbility(std::make_shared<Entity::FireballAbility>(fireball_tex, fireball_hit_tex, fireball_icon));

		// initialize player controller
		_controller = std::make_unique<PlayerController>(this, _player);

		// initialize entity manager
		_entity_manager = std::make_unique<EntityManager>();
		_entity_manager->addEntity(_player);
		_entity_manager->setPlayer(_player);

		// initialize LevelManager
		_level_manager = std::make_unique<World::LevelManager>();
		_level_manager->registerLevel(std::make_shared<World::DemoLevel>());
		_level_manager->registerLevel(std::make_shared<World::DevLevel>());
		// We no longer changeLevel here, we wait for LevelSelect.
		
		_is_running = true;

        // initialize UI
        _ui_handler = std::make_unique<Nawia::UI::UIHandler>();
        _ui_handler->initialize(_player, _entity_manager.get(), _resource_manager);

		// TEST remove later
		if (_player) {
			auto sword = _item_database.createItem(1);
			auto chest = _item_database.createItem(2);
			auto boots = _item_database.createItem(3);

			if (sword) _player->getBackpack().addItem(sword);
			if (chest) _player->getBackpack().addItem(chest);
			if (boots) _player->getBackpack().addItem(boots);
		}
	}

	Engine::~Engine()
	{
		CloseWindow();
	}

	bool Engine::isRunning() const
	{
		return _is_running && !WindowShouldClose();
	}

	std::shared_ptr<Entity::Entity> Engine::getEntityAt(const float screen_x, const float screen_y) const 
	{
		return _entity_manager->getEntityAt(screen_x, screen_y, _camera);
	}

	void Engine::spawnEntity(const std::shared_ptr<Entity::Entity> &new_entity) const 
	{
		_entity_manager->addEntity(new_entity);
	}

	void Engine::run() 
	{
		while (isRunning()) 
		{
			const float delta_time = GetFrameTime();
			handleInput();
			update(delta_time);
			render();
		}
	}

	void Engine::handleInput() 
	{
		if (!_ui_handler) return;

		if (_game_state == GameState::Menu)
		{
			const Nawia::UI::MenuAction action = _ui_handler->handleMenuInput();
            if (action == Nawia::UI::MenuAction::Play)
            {
                _previous_state = GameState::Menu;
                _ui_handler->openLevelSelect(_level_manager->getRegisteredLevels());
                _game_state = GameState::LevelSelect;
            }
            else if (action == Nawia::UI::MenuAction::Settings)
            {
                _previous_state = GameState::Menu;  // Remember where we came from
                _ui_handler->openSettings(_settings);
                _game_state = GameState::SettingsMenu;
            }
            else if (action == Nawia::UI::MenuAction::Exit)
            {
                _is_running = false;
            }
			return;
		}
		
		if (_game_state == GameState::SettingsMenu)
		{
			// ESC in Settings = go back (same as Back button)
			if (IsKeyPressed(KEY_ESCAPE)) 
			{
				_ui_handler->closeSettingsMenu();
				_game_state = _previous_state;
				if (_previous_state == GameState::Playing) 
					_show_pause_menu = true;
				
				return;
			}
	    
			const Nawia::UI::MenuAction action = _ui_handler->handleSettingsInput();
	    
			// Check if Back was clicked
			if (action == Nawia::UI::MenuAction::Play) 
			{
				// Return to previous state
				_game_state = _previous_state;
				if (_previous_state == GameState::Playing) 
					_show_pause_menu = true;  // Re-show pause menu when returning from settings

				return;
			}
	    
			// Check if settings were applied
			if (_ui_handler->wereSettingsApplied()) 
			{
				applySettings(_ui_handler->getAppliedSettings());
				_ui_handler->closeSettingsMenu();  // Reset menu to clear stale state
				// Return to previous state (not always Menu)
				_game_state = _previous_state;
				if (_previous_state == GameState::Playing) 
					_show_pause_menu = true;
			}
			return;
		}

		if (_game_state == GameState::LevelSelect)
		{
			std::string selected_lvl = _ui_handler->handleLevelSelectInput();
			
			if (selected_lvl == "BACK") {
				_ui_handler->closeLevelSelect();
				_game_state = GameState::Menu;
			}
			else if (!selected_lvl.empty()) {
				_ui_handler->closeLevelSelect();
				_level_manager->changeLevel(selected_lvl, this);
				_game_state = GameState::Playing;
			}
			return;
		}

		// Playing state - handle ESC for pause menu toggle
		if (IsKeyPressed(KEY_ESCAPE)) 
		{
		    _show_pause_menu = !_show_pause_menu;
		    return;
		}
		
		// Handle pause menu input when visible
		if (_show_pause_menu) 
		{
		    const Nawia::UI::MenuAction action = _ui_handler->handlePauseMenuInput();
		    
		    if (action == Nawia::UI::MenuAction::Play) 
			{
		        _show_pause_menu = false;  // Resume game
		    }
		    else if (action == Nawia::UI::MenuAction::Settings) 
			{
		        _previous_state = GameState::Playing;  // Remember where we came from
		        _ui_handler->openSettings(_settings);
		        _game_state = GameState::SettingsMenu;
		        _show_pause_menu = false;
		    }
		    else if (action == Nawia::UI::MenuAction::Exit) 
			{
		        _game_state = GameState::Menu;  // Quit to main menu
		        _show_pause_menu = false;
		    }
		    return;  // Don't process gameplay input while pause menu is open
		}

		// handle ui in-game input
		_ui_handler->handleInput();

		// transform mouse location to position in world
		const Vector2 mouse_pos = GetMousePosition();
		const Vector2 mouse_world_pos =  screenToIso(mouse_pos.x, mouse_pos.y, _camera.x, _camera.y);

		_entity_manager->updateHoverState(mouse_pos.x, mouse_pos.y, _camera);

		// Przekaż do levela
		_level_manager->handleInput(this);
		auto devLevel = dynamic_cast<World::DevLevel*>(_level_manager->getCurrentLevel());
		if (devLevel && devLevel->isTyping()) {
			return; // DevLevel zjada input gracza
		}

		if (!_controller)
			return;

		_controller->handleInput(mouse_world_pos.x, mouse_world_pos.y, mouse_pos.x, mouse_pos.y);
	}

	void Engine::update(const float delta_time) 
	{
		if (_game_state == GameState::Menu || _game_state == GameState::SettingsMenu || _game_state == GameState::LevelSelect)
        {
            if (_ui_handler) _ui_handler->update(delta_time);
            return;
        }

		if (!_player || !_entity_manager)
			return;

		_camera.follow(_player.get());
        if (_ui_handler) _ui_handler->update(delta_time);
        _level_manager->update(this, delta_time);
		_controller->update(delta_time);

		_entity_manager->updateEntities(delta_time);
		_entity_manager->handleEntitiesCollisions();

		// collects new entities spawned by existing ones (like projectiles) and adds them to the game world
		std::vector<std::shared_ptr<Entity::Entity>> new_spawns;
		const auto& entities = _entity_manager->getEntities(); 
		for (const auto& entity : entities)
		{
			const auto spawns = entity->getPendingSpawns();
			if (!spawns.empty())
			{
				new_spawns.insert(new_spawns.end(), spawns.begin(), spawns.end());
				entity->clearPendingSpawns();
			}
		}

		for (const auto& spawn : new_spawns)
		{
			spawnEntity(spawn);
		}
	}

	void Engine::render() const 
	{
		BeginDrawing();
		ClearBackground(BLACK);

        if (_game_state == GameState::Menu)
        {
            if (_ui_handler) _ui_handler->renderMainMenu();
        }
        else if (_game_state == GameState::SettingsMenu)
        {
            // Render main menu as background, then settings overlay
            if (_ui_handler) {
                _ui_handler->renderMainMenu();
                _ui_handler->renderSettingsMenu();
            }
        }
		else if (_game_state == GameState::LevelSelect)
		{
			if (_ui_handler) {
				_ui_handler->renderMainMenu();
				_ui_handler->renderLevelSelectMenu();
			}
		}
        else
        {
		    if (!getCurrentMap() || !_player || !_entity_manager) 
		    {
			    EndDrawing();
			    return;
		    }

		    /* RENDER START */

		    // BeginMode2D(_camera); // If we use Raylib Camera2D, otherwise manual offset

		    getCurrentMap()->render(_camera.x, _camera.y);
		    _entity_manager->renderEntities(_camera);
            
            if (_ui_handler) _ui_handler->render(_camera);
            
            // Render pause menu overlay if visible
            if (_show_pause_menu && _ui_handler) {
                _ui_handler->renderPauseMenu();
            }

			_level_manager->renderUI(const_cast<Engine*>(this));

		    /* RENDER END */
        }

		DrawFPS(10, 10);

		EndDrawing();
	}

	void Engine::applySettings(const Settings& new_settings)
	{
	    _settings = new_settings;
	    
	    // Apply resolution change
	    if (IsWindowFullscreen())
	    {
             if (!_settings.fullscreen) ToggleFullscreen(); // Turn off
             else SetWindowSize(_settings.resolution.width, _settings.resolution.height); // Update res while fullscreen
	    }
        else
        {
             if (_settings.fullscreen) 
             {
                 SetWindowSize(_settings.resolution.width, _settings.resolution.height);
                 ToggleFullscreen(); // Turn on
             }
             else
             {
                 SetWindowSize(_settings.resolution.width, _settings.resolution.height);
             }
        }
	    
	    // Apply UI scale and update global scaling
	    GlobalScaling::setManualScale(_settings.ui_scale);
	    
	    // Save settings to file
		if (_settings.save())
			Logger::debugLog("Zapisano ustawienia.");
	    
	    // Note: caller is responsible for setting _game_state to _previous_state
	}

} // namespace Nawia::Core