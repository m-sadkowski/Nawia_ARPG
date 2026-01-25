#include "Engine.h"
#include "GlobalScaling.h"
#include "Logger.h"
#include "MathUtils.h"
#include "PlayerController.h"

#include <Dummy.h>
#include <WalkingDead.h>
#include <FireballAbility.h>
#include <Projectile.h>
#include <SwordSlashAbility.h>

#include <algorithm>
#include <iostream>

#include "Checkpoint.h"
#include "Chest.h"

namespace Nawia::Core {

	Engine::Engine() : _is_running(false), _controller(nullptr), _game_state(GameState::Menu) 
	{
		SetTraceLogLevel(LOG_ERROR);
		InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Nawia");
		SetExitKey(0);  // Disable ESC = close window (we handle ESC manually)
		SetTargetFPS(60);
		
		// Load settings from file (if exists)
		if (_settings.load()) {
		    // Apply saved resolution
		    SetWindowSize(_settings.resolution.width, _settings.resolution.height);
		}
		
		// Initialize UI scaling system with saved manual scale
		GlobalScaling::setManualScale(_settings.uiScale);

		// TEMPORARY SOLUTION
		// initialize map object
		_map = std::make_unique<Map>(_resource_manager);
		_map->loadMap("map1.json");

		// initialize player
		auto player_texture = _resource_manager.getTexture("../assets/textures/player.png");
		Vector2 player_spawn_pos = _map->getPlayerSpawnPos();
		_player = std::make_shared<Entity::Player>(player_spawn_pos.x, player_spawn_pos.y, player_texture);

		// initialize spells
		const auto sword_slash_tex = _resource_manager.getTexture("../assets/textures/sword_slash.png");
		const auto sword_slash_icon = _resource_manager.getTexture("../assets/textures/icons/sword_slash_icon.png");
		_player->addAbility(std::make_shared<Entity::SwordSlashAbility>(sword_slash_tex, sword_slash_icon));
		
		const auto fireball_tex = _resource_manager.getTexture("../assets/textures/fireball.png");
		const auto fireball_hit_tex = _resource_manager.getTexture("../assets/textures/fireball_hit.png");
		const auto fireball_icon = _resource_manager.getTexture("../assets/textures/icons/fireball_icon.png");
		_player->addAbility(std::make_shared<Entity::FireballAbility>(fireball_tex, fireball_hit_tex, fireball_icon));

		// init item database
		_item_database.loadDatabase("../assets/data/items.json", _resource_manager);
		Logger::debugLog("Zaladowano baze danych przedmiotow");

		// initialize player controller
		_controller = std::make_unique<PlayerController>(this, _player);

		// initialize entity manager
		_entity_manager = std::make_unique<EntityManager>();

		// spawn test enemy
		// manual setup of a test enemy with specific abilities
		const auto enemy_tex = _resource_manager.getTexture("../assets/textures/enemy.png");
		const auto dummy = std::make_shared<Entity::Dummy>(15.0f, 15.0f, enemy_tex, 100, _map.get());
		dummy->addAbility(std::make_shared<Entity::FireballAbility>(fireball_tex, fireball_hit_tex, fireball_icon));
		dummy->setTarget(_player);
		//_entity_manager->addEntity(dummy);

		// spawn Walking Dead for testing
		const auto walking_dead = std::make_shared<Entity::WalkingDead>(18.0f, 15.0f, _map.get());
		walking_dead->setTarget(_player);
		_entity_manager->addEntity(walking_dead);

		// add player to entity manager so it can be hit by enemies
		_entity_manager->addEntity(_player);

		_is_running = true;

		const auto chest_tex = _resource_manager.getTexture("../assets/textures/chest.png");
		auto test_chest = std::make_shared<Entity::Chest>("Stara Skrzynia", 12.0f, 12.0f, chest_tex);
		auto chestplate_chest = _item_database.createItem(2);
		test_chest->addItem(chestplate_chest);
		_entity_manager->addEntity(test_chest);

		// 2. Checkpoint (InteractiveTrigger)
		auto test_checkpoint = std::make_shared<Entity::Checkpoint>("Punkt Kontrolny", 20.0f, 20.0f);
		_entity_manager->addEntity(test_checkpoint);
		_entity_manager->setPlayer(_player);

        // initialize UI
        _ui_handler = std::make_unique<Nawia::UI::UIHandler>();
        _ui_handler->initialize(_player, _entity_manager.get(), _resource_manager);

		// TEST remove later
		if (_player) {
			auto sword = _item_database.createItem(1);
			auto chest = _item_database.createItem(2);

			if (sword) _player->getBackpack().addItem(sword);
			if (chest) _player->getBackpack().addItem(chest);
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
                _game_state = GameState::Playing;
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
			if (IsKeyPressed(KEY_ESCAPE)) {
				_ui_handler->closeSettingsMenu();
				_game_state = _previous_state;
				if (_previous_state == GameState::Playing) {
					_show_pause_menu = true;
				}
				return;
			}
	    
			const Nawia::UI::MenuAction action = _ui_handler->handleSettingsInput();
	    
			// Check if Back was clicked
			if (action == Nawia::UI::MenuAction::Play) {
				// Return to previous state
				_game_state = _previous_state;
				if (_previous_state == GameState::Playing) {
					_show_pause_menu = true;  // Re-show pause menu when returning from settings
				}
				return;
			}
	    
			// Check if settings were applied
			if (_ui_handler->wereSettingsApplied()) {
				applySettings(_ui_handler->getAppliedSettings());
				_ui_handler->closeSettingsMenu();  // Reset menu to clear stale state
				// Return to previous state (not always Menu)
				_game_state = _previous_state;
				if (_previous_state == GameState::Playing) {
					_show_pause_menu = true;
				}
			}
			return;
		}

		// Playing state - handle ESC for pause menu toggle
		if (IsKeyPressed(KEY_ESCAPE)) {
		    _show_pause_menu = !_show_pause_menu;
		    return;
		}
		
		// Handle pause menu input when visible
		if (_show_pause_menu) {
		    const Nawia::UI::MenuAction action = _ui_handler->handlePauseMenuInput();
		    
		    if (action == Nawia::UI::MenuAction::Play) {
		        _show_pause_menu = false;  // Resume game
		    }
		    else if (action == Nawia::UI::MenuAction::Settings) {
		        _previous_state = GameState::Playing;  // Remember where we came from
		        _ui_handler->openSettings(_settings);
		        _game_state = GameState::SettingsMenu;
		        _show_pause_menu = false;
		    }
		    else if (action == Nawia::UI::MenuAction::Exit) {
		        _game_state = GameState::Menu;  // Quit to main menu
		        _show_pause_menu = false;
		    }
		    return;  // Don't process gameplay input while pause menu is open
		}

		// handle ui in-game input
		_ui_handler->handleInput();

		// transform mouse location to position in world
		Vector2 mouse_pos = GetMousePosition();
		Vector2 mouse_world_pos =  screenToIso(mouse_pos.x, mouse_pos.y, _camera.x, _camera.y);

		_entity_manager->updateHoverState(mouse_pos.x, mouse_pos.y, _camera);

		if (!_controller)
			return;

		_controller->handleInput(mouse_world_pos.x, mouse_world_pos.y, mouse_pos.x, mouse_pos.y);
	}

	void Engine::update(const float delta_time) 
	{
		if (_game_state == GameState::Menu || _game_state == GameState::SettingsMenu)
        {
            if (_ui_handler) _ui_handler->update(delta_time);
            return;
        }

		if (!_player || !_entity_manager)
			return;

		_camera.follow(_player.get());
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
        else
        {
		    if (!_map || !_player || !_entity_manager) 
		    {
			    EndDrawing();
			    return;
		    }

		    /* RENDER START */

		    // BeginMode2D(_camera); // If we use Raylib Camera2D, otherwise manual offset

		    _map->render(_camera.x, _camera.y);
		    _entity_manager->renderEntities(_camera);
            
            if (_ui_handler) _ui_handler->render(_camera);
            
            // Render pause menu overlay if visible
            if (_show_pause_menu && _ui_handler) {
                _ui_handler->renderPauseMenu();
            }

		    /* RENDER END */
        }

		EndDrawing();
	}

	void Engine::applySettings(const Settings& new_settings)
	{
	    _settings = new_settings;
	    
	    // Apply resolution change
	    SetWindowSize(_settings.resolution.width, _settings.resolution.height);
	    
	    // Apply UI scale and update global scaling
	    GlobalScaling::setManualScale(_settings.uiScale);
	    
	    // Save settings to file
	    _settings.save();
	    
	    // Note: caller is responsible for setting _game_state to _previous_state
	}

} // namespace Nawia::Core