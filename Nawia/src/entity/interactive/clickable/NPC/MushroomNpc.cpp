#include "MushroomNpc.h"

#include <Engine.h>
#include <EntityPathMotion.h>
#include <Logger.h>
#include <Map.h>
#include <MiniMushroomInfected.h>
#include <Quest.h>
#include <SoundIds.h>
#include <UIHandler.h>

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace Nawia::Entity {

	namespace {
		constexpr const char* MUSHROOM_MODEL = "assets/models/actors/mushroom/mushroom_raylib_fixed.glb";
		constexpr float FOLLOW_STOP_DISTANCE = 0.45f;
		constexpr float MUSHROOM_TARGET_HEIGHT = 3.6f;
		constexpr float IDLE_LOOK_AT_PLAYER_INTERVAL = 0.25f;
		constexpr float IDLE_LOOK_AT_PLAYER_RANGE = 10.0f;
		constexpr int MUSHROOM_IDLE_ANIMATION_INDEX = 0;
		constexpr int MUSHROOM_WALK_ANIMATION_INDEX = 11;
		constexpr int MUSHROOM_TALK_ANIMATION_INDEX = 15;
	}

	MushroomNpc::MushroomNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine,
		const std::string& follow_checkpoint_name)
		: StoryNpc(name, x, y, engine)
	{
		_follow_checkpoint_name = follow_checkpoint_name;
		setType(EntityType::NPCActor);
		_home_position = getCenter();
		setMovementSpeed(3.2f);

		replaceModel(MUSHROOM_MODEL, false);
		if (!hasModelLoaded()) {
			Core::Logger::errorLog("MushroomNpc: failed to load mushroom model " + std::string(MUSHROOM_MODEL));
			return;
		}

		if (fitLoadedModelToHeight(MUSHROOM_TARGET_HEIGHT))
			setAltitude(0.0f);

		addAnimation("idle", MUSHROOM_MODEL, MUSHROOM_IDLE_ANIMATION_INDEX);
		addAnimation("walk", MUSHROOM_MODEL, MUSHROOM_WALK_ANIMATION_INDEX);
		addAnimation("talk", MUSHROOM_MODEL, MUSHROOM_TALK_ANIMATION_INDEX);
		playIdleAnimation();

		setPlaceholderDialogue("Gzib", "Jeszcze ustawimy tu prawdziwy dialog Gziba.");
	}

	bool MushroomNpc::shouldNotifyQuestTalkOnDialogueComplete() const {
		const std::string& stage = getDialogueStageKey();
		return stage == "intro" || stage == "follow" || stage == "report";
	}

	void MushroomNpc::onInteract(Entity& instigator) {
		rotateTowardsCenter(instigator.getCenter().x, instigator.getCenter().y);
		instigator.rotateTowardsCenter(getCenter().x, getCenter().y);

		refreshDialogue();
		_playing_talk = true;
		playTalkAnimation();
	}

	bool MushroomNpc::canInteract() const {
		return hasDialogueAvailable();
	}

	void MushroomNpc::update(const float delta_time) {
		if (isDormant())
			return;

		Entity::update(delta_time);

		if (isAnimationLocked())
			return;

		if (_playing_talk) {
			const bool dialogue_open = _engine && _engine->getUIHandler().isDialogueOpen();
			if (!dialogue_open) {
				_playing_talk = false;
				playIdleAnimation();
			}

			rotateToPlayerOnInterval(delta_time);
			updateMovementSound(Audio::SoundPath::GzibWalk, false);
			return;
		}

		updateCompanionTravel(delta_time);
		updateMovementSound(Audio::SoundPath::GzibWalk, isMoving(), 0.42f, 1.12f);

		if (isMoving()) {
			playWalkAnimation();
		} else {
			rotateToPlayerOnInterval(delta_time);
			playIdleAnimation();
		}
	}

	void MushroomNpc::handleQuestTalkCompleted(Core::Engine& engine) {
		if (getLastCompletedDialogueStage() == "report" && !_return_started) {
			startRoute(TravelMode::ReturningHome);
			return;
		}

		if (getLastCompletedDialogueStage() != "follow")
			return;

		const int rescued_count = getRescuedMushroomCount();
		if (rescued_count <= 0)
			return;

		for (int i = 0; i < rescued_count; ++i)
			engine.getQuestManager().notifyKill("Robal");

		engine.getQuestManager().update(&engine);
	}

	void MushroomNpc::refreshDialogue() {
		if (!_engine)
			return;

		const auto* intro_quest = _engine->getQuestManager().getQuest("gzib_talk_intro");
		const auto* follow_quest = _engine->getQuestManager().getQuest("gzib_follow");
		const auto* report_quest = _engine->getQuestManager().getQuest("gzib_report_brothers");
		std::string stage_key = "idle";

		if (getDialogueStartNode() > 0 && getDialogueStageKey() == "report") {
			setDialogue(buildDialogueFromConfig("gzib_report"));
			return;
		}

		if (getDialogueStartNode() > 0 && getDialogueStageKey() == "follow") {
			setDialogue(buildDialogueFromConfig("gzib_follow"));
			return;
		}

		if (getDialogueStartNode() > 0 && getDialogueStageKey() == "intro") {
			setDialogue(buildDialogueFromConfig("gzib_intro"));
			return;
		}

		if (report_quest && report_quest->isActive()) {
			stage_key = "report";
			setDialogueStageKey(stage_key);
			setDialogue(buildDialogueFromConfig("gzib_report"));
			return;
		}

		const bool follow_checkpoint_ready = follow_quest && follow_quest->isActive() &&
			(_reached_follow_checkpoint ||
			 (!follow_quest->objectives.empty() && follow_quest->objectives.front().isCompleted()));
		if (follow_checkpoint_ready) {
			stage_key = "follow";
			setDialogueStageKey(stage_key);
			if (areAllMushroomsAlreadyRescued()) {
				setDialogue(buildDialogueFromConfig("gzib_follow_done"));
			} else {
				setDialogue(buildDialogueFromConfig("gzib_follow"));
			}
			return;
		}

		if (!intro_quest || intro_quest->isActive() || intro_quest->isAvailable()) {
			stage_key = "intro";
			setDialogueStageKey(stage_key);
			setDialogue(buildDialogueFromConfig("gzib_intro"));
			return;
		}

		setDialogueStageKey(stage_key);
		setPlaceholderDialogue("Gzib", "GZIBY CZEKAJA.");
	}

	void MushroomNpc::updateCompanionTravel(const float delta_time) {
		if (!_engine)
			return;

		if (_travel_mode == TravelMode::None) {
			if (_reached_follow_checkpoint)
				return;

			const auto* follow_quest = _engine->getQuestManager().getQuest("gzib_follow");
			if (!follow_quest || !follow_quest->isActive())
				return;

			startRoute(TravelMode::ToBrothers);
			if (_travel_mode == TravelMode::None)
				return;
		}

		if (_travel_waypoint_index >= _travel_waypoints.size()) {
			stopPathMovement();
			_travel_mode = TravelMode::None;
			return;
		}

		const Vector2 target = _travel_waypoints[_travel_waypoint_index];
		const float distance = Vector2Distance(getCenter(), target);
		if (distance <= FOLLOW_STOP_DISTANCE) {
			advanceRoute();
			return;
		}

		if (!_follow_path_requested) {
			buildPathToPoint(target);
			_follow_path_requested = true;
		}

		updatePathMovement(delta_time);
		if (isMoving())
			playWalkAnimation();
	}

	void MushroomNpc::startRoute(const TravelMode mode) {
		_travel_waypoints = collectOrderedFollowWaypoints(mode == TravelMode::ReturningHome);
		if (mode == TravelMode::ReturningHome)
			_travel_waypoints.push_back(_home_position);

		_travel_waypoint_index = 0;
		_travel_mode = _travel_waypoints.empty() ? TravelMode::None : mode;
		_follow_path_requested = false;
		_return_started = mode == TravelMode::ReturningHome;

		if (_travel_mode != TravelMode::None) {
			if (mode == TravelMode::ReturningHome)
				sendPurifiedFollowersHome();
			buildPathToPoint(_travel_waypoints.front());
		}
	}

	void MushroomNpc::advanceRoute() {
		_travel_waypoint_index++;
		_follow_path_requested = false;

		if (_travel_waypoint_index < _travel_waypoints.size()) {
			buildPathToPoint(_travel_waypoints[_travel_waypoint_index]);
			return;
		}

		stopPathMovement();
		const TravelMode finished_mode = _travel_mode;
		_travel_mode = TravelMode::None;

		if (finished_mode == TravelMode::ToBrothers) {
			_reached_follow_checkpoint = true;
			_engine->getQuestManager().notifyCheckpointReached(_follow_checkpoint_name);
		}

		playIdleAnimation();
	}

	void MushroomNpc::buildPathToPoint(const Vector2 target) {
		const std::size_t nav_path_size = PathMotion::buildPathToPoint(
			*this,
			_engine ? _engine->getCurrentMap() : nullptr,
			target,
			_current_path);
		Core::Logger::debugLog("MushroomNpc: sciezka Gziba ma " + std::to_string(nav_path_size) + " punktow");
	}

	void MushroomNpc::updatePathMovement(const float delta_time) {
		PathMotion::updatePathMovement(*this, delta_time, _current_path);
	}

	void MushroomNpc::stopPathMovement() {
		PathMotion::stopPathMovement(*this, _current_path);
	}

	void MushroomNpc::sendPurifiedFollowersHome() {
		if (!_engine)
			return;

		for (const auto& entity : _engine->getEntityManager().getEntities()) {
			const auto mushroom = std::dynamic_pointer_cast<MiniMushroomInfected>(entity);
			if (!mushroom || !mushroom->isPurified())
				continue;

			const float angle = static_cast<float>(GetRandomValue(0, 628)) / 100.0f;
			const float radius = static_cast<float>(GetRandomValue(120, 500)) / 100.0f;
			const Vector2 destination = {
				_home_position.x + std::cos(angle) * radius,
				_home_position.y + std::sin(angle) * radius
			};

			std::vector<Vector2> route = _travel_waypoints;
			route.push_back(destination);
			mushroom->setPropRoute(route);
		}
	}

	void MushroomNpc::rotateToPlayerOnInterval(const float delta_time) {
		_look_at_player_timer -= delta_time;
		if (_look_at_player_timer > 0.0f)
			return;

		_look_at_player_timer = IDLE_LOOK_AT_PLAYER_INTERVAL;
		if (const auto player = _engine ? _engine->getPlayer() : nullptr) {
			if (Vector2DistanceSqr(getCenter(), player->getCenter()) > IDLE_LOOK_AT_PLAYER_RANGE * IDLE_LOOK_AT_PLAYER_RANGE)
				return;
			rotateTowardsCenter(player->getCenter().x, player->getCenter().y);
		}
	}

	void MushroomNpc::playIdleAnimation() {
		if (getAnimationFrameCount("idle") > 0)
			playAnimation("idle", true, false);
	}

	void MushroomNpc::playWalkAnimation() {
		if (getAnimationFrameCount("walk") > 0)
			playAnimation("walk", true, false);
	}

	void MushroomNpc::playTalkAnimation() {
		if (getAnimationFrameCount("talk") > 0)
			playAnimation("talk", true, false, 0, true);
	}

	std::vector<Vector2> MushroomNpc::collectOrderedFollowWaypoints(const bool reverse_to_home) const {
		std::vector<Vector2> remaining;
		if (!_engine)
			return remaining;

		for (const auto& entity : _engine->getEntityManager().getEntities()) {
			if (entity && entity->getName() == _follow_checkpoint_name)
				remaining.push_back(entity->getCenter());
		}

		std::vector<Vector2> ordered;
		Vector2 cursor = reverse_to_home && !remaining.empty() ? _home_position : getCenter();
		if (reverse_to_home && !remaining.empty()) {
			std::vector<Vector2> from_home;
			while (!remaining.empty()) {
				auto nearest_it = std::min_element(remaining.begin(), remaining.end(), [cursor](const Vector2& a, const Vector2& b) {
					return Vector2DistanceSqr(cursor, a) < Vector2DistanceSqr(cursor, b);
				});
				cursor = *nearest_it;
				from_home.push_back(cursor);
				remaining.erase(nearest_it);
			}
			ordered.assign(from_home.rbegin(), from_home.rend());
			return ordered;
		}

		while (!remaining.empty()) {
			auto nearest_it = std::min_element(remaining.begin(), remaining.end(), [cursor](const Vector2& a, const Vector2& b) {
				return Vector2DistanceSqr(cursor, a) < Vector2DistanceSqr(cursor, b);
			});
			cursor = *nearest_it;
			ordered.push_back(cursor);
			remaining.erase(nearest_it);
		}

		return ordered;
	}

	int MushroomNpc::getRescuedMushroomCount() const {
		if (!_engine)
			return 0;

		int rescued_count = 0;
		for (const auto& entity : _engine->getEntityManager().getEntities()) {
			const auto mushroom = std::dynamic_pointer_cast<MiniMushroomInfected>(entity);
			if (mushroom && mushroom->isPurified())
				rescued_count++;
		}

		return rescued_count;
	}

	int MushroomNpc::getRequiredRescueCount() const {
		if (!_engine)
			return 5;

		const auto* rescue_quest = _engine->getQuestManager().getQuest("gzib_rescue_brothers");
		if (!rescue_quest)
			return 5;

		for (const auto& objective : rescue_quest->objectives) {
			if (objective.target_name == "Robal")
				return objective.required_count;
		}

		return 5;
	}

	bool MushroomNpc::areAllMushroomsAlreadyRescued() const {
		const int rescued_count = getRescuedMushroomCount();
		return rescued_count > 0 && rescued_count >= getRequiredRescueCount();
	}

	bool MushroomNpc::hasDialogueAvailable() const {
		if (!_engine)
			return false;

		const auto* intro_quest = _engine->getQuestManager().getQuest("gzib_talk_intro");
		if (!intro_quest || intro_quest->isActive() || intro_quest->isAvailable())
			return true;

		const auto* follow_quest = _engine->getQuestManager().getQuest("gzib_follow");
		const bool follow_checkpoint_ready = follow_quest && follow_quest->isActive() &&
			(_reached_follow_checkpoint ||
			 (!follow_quest->objectives.empty() && follow_quest->objectives.front().isCompleted()));
		if (follow_checkpoint_ready)
			return true;

		const auto* report_quest = _engine->getQuestManager().getQuest("gzib_report_brothers");
		return report_quest && report_quest->isActive();
	}

} // namespace Nawia::Entity
