#include "LevelManager.h"

#include <Engine.h>
#include <Logger.h>

namespace Nawia::World {

	LevelManager::~LevelManager() {
		_levels.clear();
	}

	void LevelManager::registerLevel(std::shared_ptr<Level> level) {
		if (level) {
			_levels[level->getName()] = level;
		}
	}

	void LevelManager::changeLevel(const std::string& name, Core::Engine* engine) {
		auto it = _levels.find(name);
		if (it != _levels.end()) {
			if (_current_level) {
				_current_level->onExit(engine);
			}

			_current_level = it->second;
			
			if (_current_level) {
				_current_level->onEnter(engine);
			}

			Core::Logger::debugLog("Zmieniono poziom na: " + name);
		} else {
			Core::Logger::errorLog("Nie znaleziono poziomu: " + name);
		}
	}

	std::vector<std::string> LevelManager::getRegisteredLevels() const {
		std::vector<std::string> names;
		names.reserve(_levels.size());
		for (const auto& [name, level] : _levels) {
			names.push_back(name);
		}
		return names;
	}

	Level* LevelManager::getCurrentLevel() const {
		return _current_level.get();
	}

	void LevelManager::handleInput(Core::Engine* engine) {
		if (_current_level) _current_level->handleInput(engine);
	}

	void LevelManager::update(Core::Engine* engine, float dt) {
		if (_current_level) _current_level->update(engine, dt);
	}

	void LevelManager::renderUI(Core::Engine* engine) {
		if (_current_level) _current_level->renderUI(engine);
	}

} // namespace Nawia::World
