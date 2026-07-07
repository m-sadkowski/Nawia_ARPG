#include "FirstLevel.h"

#include "FirstLevelInternal.h"

#include <AudioManager.h>
#include <Engine.h>
#include <LocationJsonLoader.h>
#include <Logger.h>
#include <Map.h>

namespace Nawia::World {

	namespace F = FirstLevelSupport;

	std::vector<std::string> FirstLevel::getLocations() const {
		const auto loaded = Level::getLocations();
		if (!loaded.empty())
			return loaded;

		std::vector<std::string> names;
		for (const auto& location_file : F::locationFiles()) {
			LocationDefinition definition;
			if (LocationJsonLoader::loadLocation(location_file.path, definition))
				names.push_back(definition.name);
		}
		return names;
	}

	std::vector<LevelLocationFile> FirstLevel::getLocationFiles() const {
		return F::locationFiles();
	}

	void FirstLevel::changeLocation(Core::Engine* engine, const std::string& location_name) {
		const std::string resolved_location =
			location_name == "Wczora" && !_location_definitions.empty()
				? _location_definitions.front().name
				: location_name;

		Level::changeLocation(engine, resolved_location);

		if (!engine || _current_location_index >= _location_definitions.size())
			return;

		engine->getLightingSystem().loadLightingFromJson(F::getLightingFileForLocation(_location_definitions[_current_location_index]));
		if (engine->getCurrentMap())
			engine->getLightingSystem().applyToModel(engine->getCurrentMap()->getModel());
	}

	void FirstLevel::prepareForRespawn(Core::Engine* engine) {
		Level::prepareForRespawn(engine);
		if (!engine || _current_location_index >= _location_definitions.size())
			return;

		engine->getLightingSystem().loadLightingFromJson(F::getLightingFileForLocation(_location_definitions[_current_location_index]));
		if (engine->getCurrentMap())
			engine->getLightingSystem().applyToModel(engine->getCurrentMap()->getModel());
	}

	void FirstLevel::onEnter(Core::Engine* engine) {
		Core::Logger::debugLog("Ladowanie poziomu Wczora...");
		activatePreparedLocations(engine);

		if (engine) {
			if (_current_location_index < _location_definitions.size())
				engine->getLightingSystem().loadLightingFromJson(F::getLightingFileForLocation(_location_definitions[_current_location_index]));
			if (engine->getCurrentMap())
				engine->getLightingSystem().applyToModel(engine->getCurrentMap()->getModel());
			engine->getAudioManager().playMusic(F::MUSIC_PATH, true, 0.65f);
		}
	}

	void FirstLevel::onExit(Core::Engine* engine) {
		finishIntroSequence(engine);
		Level::onExit(engine);
	}

	void FirstLevel::onNewGameStarted(Core::Engine* engine) {
		equipPresentationBoots(engine);
		startIntroSequence(engine);
	}

	void FirstLevel::handleStoryEvent(Core::Engine* engine, const std::string& event_id, const Vector2 world_position) {
		if (event_id == "wanda_corpse_inspected")
			queueCorpseInspected(world_position);
		else if (event_id == "wczora_outro_requested")
			startOutroSequence(engine);
	}

	bool FirstLevel::blocksPlayerControl() const {
		return _intro_phase != IntroPhase::Inactive && _intro_phase != IntroPhase::InspectCorpse;
	}

	bool FirstLevel::isInteractionOnly() const {
		return _intro_phase == IntroPhase::InspectCorpse;
	}

	float FirstLevel::getCameraZoomMultiplier() const {
		return _intro_phase == IntroPhase::Inactive ? 1.0f : F::INTRO_CAMERA_ZOOM_FACTOR;
	}

	float FirstLevel::getCameraTargetHeightMultiplier() const {
		return _intro_phase == IntroPhase::Inactive ? 1.0f : F::INTRO_CAMERA_TARGET_HEIGHT_FACTOR;
	}

} // namespace Nawia::World
