#include "Engine.h"
#include "GlobalScaling.h"
#include "Logger.h"
#include "MathUtils.h"
#include "PlayerController.h"

#include <FireballAbility.h>
#include <SwordSlashAbility.h>
#include <DemoLevel.h>
#include <DevLevel.h>
#include <MrocznyLasLevel.h>
#include <StarozytneLochyLevel.h>
#include <PobojowiskoLevel.h>
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

		// init quest system
		_quest_manager.loadFromJson("../assets/data/quests.json");
		
		// init boss system
		_boss_manager.loadFromJson("../assets/data/bosses.json");

		// initialize player
		Vector2 player_spawn_pos = {0.0f, 0.0f};
		_player = Entity::PlayerBuilder(this).setPosition(player_spawn_pos).build();

		// initialize spells
		const auto sword_slash_tex = _resource_manager.getTexture("../assets/textures/sword_slash.png");
		const auto sword_slash_icon = _resource_manager.getTexture("../assets/textures/icons/sword_slash_icon.png");
		_player->addAbility(std::make_shared<Entity::SwordSlashAbility>(sword_slash_tex, sword_slash_icon));
		
		const auto fireball_hit_tex = _resource_manager.getTexture("../assets/textures/fireball_hit.png");
		const auto fireball_icon = _resource_manager.getTexture("../assets/textures/icons/fireball_icon.png");
		_player->addAbility(std::make_shared<Entity::FireballAbility>("../assets/models/fireball.glb", 0.5f, fireball_hit_tex, fireball_icon));

		// initialize player controller
		_controller = std::make_unique<PlayerController>(this, _player);

		// initialize entity manager
		_entity_manager = std::make_unique<EntityManager>(this);
		_entity_manager->addEntity(_player);
		_entity_manager->setPlayer(_player);

		// initialize LevelManager
		_level_manager = std::make_unique<World::LevelManager>();
		_level_manager->registerLevel(std::make_shared<World::DemoLevel>());
		_level_manager->registerLevel(std::make_shared<World::DevLevel>());
		_level_manager->registerLevel(std::make_shared<World::MrocznyLasLevel>());
		_level_manager->registerLevel(std::make_shared<World::StarozytneLochyLevel>());
		_level_manager->registerLevel(std::make_shared<World::PobojowiskoLevel>());
		
		_is_running = true;

        // initialize UI
        _ui_handler = std::make_unique<Nawia::UI::UIHandler>();
        _ui_handler->initialize(_player, _entity_manager.get(), _resource_manager, &_quest_manager);
        _ui_handler->setLevelManager(_level_manager.get());

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
		return _entity_manager->getEntityAt(screen_x, screen_y, _camera.get());
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
                _ui_handler->openLevelSelect(_level_manager->getRegisteredLevelInfos());
                _game_state = GameState::LevelSelect;
            }
            else if (action == Nawia::UI::MenuAction::Settings)
            {
                _previous_state = GameState::Menu;
                _ui_handler->openSettings(_settings);
                _game_state = GameState::SettingsMenu;
            }
            else if (action == Nawia::UI::MenuAction::Exit)
            {
                _is_running = false;
            }
			return;
		}

		if (_game_state == GameState::GameOver) {
			const Nawia::UI::MenuAction action = _ui_handler->handleGameOverInput();
			if (action == Nawia::UI::MenuAction::Respawn) {
				_player->respawn();
				_entity_manager->addEntity(_player);
				_game_state = GameState::Playing;
			}
			else if (action == Nawia::UI::MenuAction::Exit) {
				_game_state = GameState::Menu;
			}
			return;
		}
		
		if (_game_state == GameState::SettingsMenu)
		{
			if (IsKeyPressed(KEY_ESCAPE)) 
			{
				_ui_handler->closeSettingsMenu();
				_game_state = _previous_state;
				if (_previous_state == GameState::Playing) 
					_show_pause_menu = true;
				
				return;
			}
	    
			const Nawia::UI::MenuAction action = _ui_handler->handleSettingsInput();
	    
			if (action == Nawia::UI::MenuAction::Play) 
			{
				_game_state = _previous_state;
				if (_previous_state == GameState::Playing) 
					_show_pause_menu = true;

				return;
			}
	    
			if (_ui_handler->wereSettingsApplied()) 
			{
				applySettings(_ui_handler->getAppliedSettings());
				_ui_handler->closeSettingsMenu();
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
		        _show_pause_menu = false;
		    }
		    else if (action == Nawia::UI::MenuAction::Settings) 
			{
		        _previous_state = GameState::Playing;
		        _ui_handler->openSettings(_settings);
		        _game_state = GameState::SettingsMenu;
		        _show_pause_menu = false;
		    }
		    else if (action == Nawia::UI::MenuAction::Exit) 
			{
		        _game_state = GameState::Menu;
		        _show_pause_menu = false;
		    }
		    return;
		}

		// handle ui in-game input
		_ui_handler->handleInput();

		// transform mouse location to world position using ray-cast
		const Vector2 mouse_pos = GetMousePosition();
		const Vector2 mouse_world_pos = screenToWorld(_camera.get(), mouse_pos.x, mouse_pos.y);

		_entity_manager->updateHoverState(mouse_pos.x, mouse_pos.y, _camera.get());

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

		if (_player->isDead()) {
			if (_boss_manager.isFightActive()) {
				_boss_manager.endBossFight(false, this);
			}
			_game_state = GameState::GameOver;
			return;
		}

		_camera.follow(_player.get());
        if (_ui_handler) _ui_handler->update(delta_time);
        _level_manager->update(this, delta_time);
		_controller->update(delta_time);

		_entity_manager->updateEntities(delta_time);
		_entity_manager->handleEntitiesCollisions();

		// Update quest states (unlock chains, auto-complete)
		_quest_manager.update(this);

		// Update boss states
		_boss_manager.update(this, delta_time);

		// collects new entities spawned by existing ones (like projectiles)
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
		ClearBackground(Color{30, 30, 35, 255});


        if (_game_state == GameState::Menu && _ui_handler)
        {
            _ui_handler->renderMainMenu();
        }
        else if (_game_state == GameState::SettingsMenu && _ui_handler)
        {
            _ui_handler->renderMainMenu();
            _ui_handler->renderSettingsMenu();
        }
		else if (_game_state == GameState::LevelSelect && _ui_handler)
		{
			_ui_handler->renderMainMenu();
			_ui_handler->renderLevelSelectMenu();
		}
		else if (_game_state == GameState::GameOver)
		{
			// Render game world behind the overlay
			if (getCurrentMap() && _player && _entity_manager) {
				BeginMode3D(_camera.get());
				getCurrentMap()->render();
				_entity_manager->renderEntities(_camera.get());
				EndMode3D();
			}
			if (_ui_handler) _ui_handler->renderGameOverScreen();
		}
        else
        {
		    if (!getCurrentMap() || !_player || !_entity_manager) 
		    {
			    EndDrawing();
			    return;
		    }

		    /* RENDER 3D SCENE */
		    BeginMode3D(_camera.get());

		    getCurrentMap()->render();
		    _entity_manager->renderEntities(_camera.get());

		    EndMode3D();

		    /* RENDER 2D UI OVERLAY */
            if (_ui_handler) _ui_handler->render(_camera, &_boss_manager);
            
            // Render pause menu overlay if visible
            if (_show_pause_menu && _ui_handler)
                _ui_handler->renderPauseMenu();

			_level_manager->renderUI(const_cast<Engine*>(this));
        }

		DrawFPS(10, 10);

		EndDrawing();
	}

	void Engine::applySettings(const Settings& new_settings)
	{
	    _settings = new_settings;
	    
	    if (IsWindowFullscreen())
	    {
             if (!_settings.fullscreen) ToggleFullscreen();
             else SetWindowSize(_settings.resolution.width, _settings.resolution.height);
	    }
        else
        {
             if (_settings.fullscreen) 
             {
                 SetWindowSize(_settings.resolution.width, _settings.resolution.height);
                 ToggleFullscreen();
             }
             else
             {
                 SetWindowSize(_settings.resolution.width, _settings.resolution.height);
             }
        }
	    
	    GlobalScaling::setManualScale(_settings.ui_scale);
	    
		if (_settings.save())
			Logger::debugLog("Zapisano ustawienia.");
	}

} // namespace Nawia::Core