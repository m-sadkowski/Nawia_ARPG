#include "MushroomNpc.h"

#include <Engine.h>
#include <Logger.h>
#include <Map.h>
#include <MiniMushroomInfected.h>
#include <Quest.h>
#include <SoundIds.h>
#include <UIHandler.h>

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

	namespace {
		constexpr const char* MUSHROOM_MODEL = "assets/models/actors/gzib/mushroom_raylib_fixed.glb";
		constexpr const char* MUSHROOM_FALLBACK_MODEL = "assets/models/actors/cat/cat_bounce.glb";
		constexpr float FOLLOW_STOP_DISTANCE = 0.45f;
		constexpr float MUSHROOM_TARGET_HEIGHT = 3.6f;
		constexpr float MUSHROOM_FALLBACK_SCALE = 15.0f;
		constexpr float MUSHROOM_MAX_SCALE = 50000.0f;
		constexpr float IDLE_LOOK_AT_PLAYER_INTERVAL = 0.25f;
	}

	MushroomNpc::MushroomNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine,
		const std::string& follow_checkpoint_name)
		: StoryNpc(name, x, y)
	{
		setEngine(engine);
		configure(follow_checkpoint_name);
	}

	bool MushroomNpc::shouldNotifyQuestTalkOnDialogueComplete() const {
		const std::string& stage = getDialogueStageKey();
		return stage == "intro" || stage == "follow" || stage == "report";
	}

	void MushroomNpc::onInteract(Entity& instigator) {
		rotateTowardsCenter(instigator.getCenter().x, instigator.getCenter().y);
		instigator.rotateTowardsCenter(getCenter().x, getCenter().y);

		refreshDialogue();

		if (_use_procedural_mushroom_animation) {
			_playing_talk = true;
			return;
		}

		if (getAnimationFrameCount("talk") > 0) {
			_playing_talk = true;
			playAnimation("talk", true, false, 0, true);
		} else if (getAnimationFrameCount("Interact") > 0) {
			_playing_talk = true;
			playAnimation("Interact", false, true, 0, true);
		} else if (getAnimationFrameCount("Wave") > 0) {
			_playing_talk = true;
			playAnimation("Wave", false, true, 0, true);
		}
	}

	bool MushroomNpc::canInteract() const {
		return hasDialogueAvailable();
	}

	void MushroomNpc::update(const float delta_time) {
		if (isDormant())
			return;

		Entity::update(delta_time);

		if (_pending_standup_after_die && !isAnimationLocked()) {
			_pending_standup_after_die = false;
			if (getAnimationFrameCount("stand_up") > 0)
				playAnimation("stand_up", false, true, 0, true);
			return;
		}

		if (isAnimationLocked())
			return;

		if (_playing_talk) {
			const bool dialogue_open = _engine && _engine->getUIHandler().isDialogueOpen();
			if (!dialogue_open) {
				_playing_talk = false;
				if (getAnimationFrameCount("idle") > 0)
					playAnimation("idle");
			}

			rotateToPlayerOnInterval(delta_time);
			updateProceduralIdleMotion(delta_time);
			updateMovementSound(Audio::SoundPath::GzibWalk, false);
			return;
		}

		updateCompanionTravel(delta_time);
		updateMovementSound(Audio::SoundPath::GzibWalk, _is_moving, 0.42f, 1.12f);

		if (_use_procedural_mushroom_animation) {
			if (!_is_moving)
				rotateToPlayerOnInterval(delta_time);
			updateProceduralIdleMotion(delta_time);
		} else if (!_is_moving && getAnimationFrameCount("idle") > 0) {
			playAnimation("idle");
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

		if (areAllMushroomsAlreadyRescued() && getAnimationFrameCount("die") > 0) {
			_playing_talk = false;
			_pending_standup_after_die = getAnimationFrameCount("stand_up") > 0;
			playAnimation("die", false, true, 0, true);
		}

		for (int i = 0; i < rescued_count; ++i)
			engine.getQuestManager().notifyKill("Robal");

		engine.getQuestManager().update(&engine);
	}

	void MushroomNpc::configure(const std::string& follow_checkpoint_name) {
		_follow_checkpoint_name = follow_checkpoint_name;
		_type = EntityType::NPCActor;
		_home_position = getCenter();
		setMovementSpeed(3.2f);

		bool using_fallback_model = false;
		replaceModel(MUSHROOM_MODEL, false);
		if (!hasModelLoaded()) {
			Core::Logger::errorLog("MushroomNpc: failed to load mushroom model " + std::string(MUSHROOM_MODEL));
			using_fallback_model = true;
			loadModel(MUSHROOM_FALLBACK_MODEL, false);
		}

		if (hasModelLoaded()) {
			const BoundingBox bounds = GetModelBoundingBox(getModel());
			const float model_height = bounds.max.y - bounds.min.y;
			float computed_scale = MUSHROOM_FALLBACK_SCALE;
			if (model_height > 1e-8f)
				computed_scale = std::clamp(MUSHROOM_TARGET_HEIGHT / model_height, 0.1f, MUSHROOM_MAX_SCALE);

			setScale(computed_scale);

			const float center_x = 0.5f * (bounds.min.x + bounds.max.x);
			const float center_z = 0.5f * (bounds.min.z + bounds.max.z);
			getModel().transform = MatrixMultiply(
				MatrixTranslate(-center_x, -bounds.min.y, -center_z),
				getModel().transform);
			setAltitude(0.0f);
		}

		if (!using_fallback_model) {
			addAnimation("talk", MUSHROOM_MODEL, 0);
			addAnimation("stand_up", MUSHROOM_MODEL, 4);
			addAnimation("die", MUSHROOM_MODEL, 6);
			addAnimation("idle", MUSHROOM_MODEL, 8);
			addAnimation("run", MUSHROOM_MODEL, 9);
			addAnimation("walk", MUSHROOM_MODEL, 15);
			addAnimation("walk_slow", MUSHROOM_MODEL, 11);

			_use_procedural_mushroom_animation = getAnimationFrameCount("idle") <= 0;
			if (_use_procedural_mushroom_animation) {
				_base_altitude = getAltitude();
				_procedural_base_altitude_initialized = false;
			} else {
				playAnimation("idle", true, false, 0, true);
			}
		} else {
			playAnimation("default");
		}

		setPlaceholderDialogue("Gzib", "Jeszcze ustawimy tu prawdziwy dialog Gziba.");
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

		if (_is_moving && !_use_procedural_mushroom_animation && getAnimationFrameCount("walk") > 0)
			playAnimation("walk");
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

		if (!_use_procedural_mushroom_animation && getAnimationFrameCount("idle") > 0)
			playAnimation("idle");
	}

	void MushroomNpc::buildPathToPoint(const Vector2 target) {
		_current_path.clear();

		if (_engine && _engine->getCurrentMap() && _engine->getCurrentMap()->getNavMesh().isReady()) {
			_current_path = _engine->getCurrentMap()->findPath(getWorldPos3D(), {target.x, getAltitude(), target.y});
			Core::Logger::debugLog("MushroomNpc: sciezka Gziba ma " + std::to_string(_current_path.size()) + " punktow");
		}

		if (_current_path.empty())
			_current_path.push_back(target);

		trimCurrentPathStart();

		if (!_current_path.empty())
			moveTo(_current_path.front().x, _current_path.front().y);
		else
			stopPathMovement();
	}

	void MushroomNpc::trimCurrentPathStart() {
		if (_current_path.empty())
			return;

		const Vector2 first_path_point = _current_path.front();
		const float dx = first_path_point.x - getCenter().x;
		const float dy = first_path_point.y - getCenter().y;
		if (dx * dx + dy * dy < 0.1f)
			_current_path.erase(_current_path.begin());
	}

	void MushroomNpc::updatePathMovement(const float delta_time) {
		if (!_is_moving && !_current_path.empty()) {
			_current_path.erase(_current_path.begin());

			if (!_current_path.empty())
				moveTo(_current_path.front().x, _current_path.front().y);
		}

		updateMovement(delta_time);
	}

	void MushroomNpc::stopPathMovement() {
		_current_path.clear();
		_is_moving = false;
		setVelocity(0.0f, 0.0f);
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

	void MushroomNpc::updateProceduralIdleMotion(const float delta_time) {
		if (!_procedural_base_altitude_initialized) {
			_base_altitude = getAltitude();
			_procedural_base_altitude_initialized = true;
		}

		_procedural_anim_time += delta_time;

		float bob = 0.0f;
		if (_playing_talk) {
			bob = 0.045f * std::sin(_procedural_anim_time * 7.0f);
		} else if (_is_moving) {
			bob = 0.075f * std::abs(std::sin(_procedural_anim_time * 8.5f));
		} else {
			bob = 0.03f * std::sin(_procedural_anim_time * 2.2f);
		}

		setAltitude(_base_altitude + bob);
	}

	void MushroomNpc::rotateToPlayerOnInterval(const float delta_time) {
		_look_at_player_timer -= delta_time;
		if (_look_at_player_timer > 0.0f)
			return;

		_look_at_player_timer = IDLE_LOOK_AT_PLAYER_INTERVAL;
		if (const auto player = _engine ? _engine->getPlayer() : nullptr)
			rotateTowardsCenter(player->getCenter().x, player->getCenter().y);
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
