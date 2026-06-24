#include "LevelManager.h"

#include <Engine.h>
#include <Level.h>
#include <Logger.h>

#include <utility>

namespace Nawia::World {

	LevelManager::~LevelManager() {
		_levels.clear();
	}

	void LevelManager::registerLevel(std::shared_ptr<Level> level) {
		if (level) {
			_levels[level->getName()] = std::move(level);
		}
	}

	void LevelManager::changeLevel(const std::string& name, Core::Engine* engine) {
		auto it = _levels.find(name);
		if (it == _levels.end()) {
			Core::Logger::errorLog("Nie znaleziono poziomu: " + name);
			return;
		}

		if (_current_level) {
			_current_level->onExit(engine);
		}

		_current_level = it->second;
		_current_level->onEnter(engine);

		// Informuje questy o aktualnym poziomie, zeby mogly odblokowac zadania.
		if (engine) {
			engine->getQuestManager().setCurrentLevel(_current_level->getName());
		}

		Core::Logger::debugLog("Zmieniono poziom na: " + name);
	}

	std::vector<LevelInfo> LevelManager::getRegisteredLevelInfos() const {
		std::vector<LevelInfo> infos;
		infos.reserve(_levels.size());
		for (const auto& [name, level] : _levels) {
			infos.push_back({name, level->getLocations(), level->allowsSaves()});
		}
		return infos;
	}

	Level* LevelManager::getCurrentLevel() const {
		return _current_level.get();
	}

	std::shared_ptr<Level> LevelManager::getRegisteredLevel(const std::string& name) const {
		const auto level_it = _levels.find(name);
		if (level_it == _levels.end())
			return nullptr;

		return level_it->second;
	}

	std::string LevelManager::getCurrentLevelName() const {
		return _current_level ? _current_level->getName() : "";
	}

	std::string LevelManager::getCurrentLocationName() const {
		return _current_level ? _current_level->getCurrentLocationName() : "";
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
