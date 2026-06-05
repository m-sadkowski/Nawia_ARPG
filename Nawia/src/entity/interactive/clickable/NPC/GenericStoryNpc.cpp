#include "GenericStoryNpc.h"

#include <Engine.h>
#include <EntityManager.h>
#include <Logger.h>
#include <Map.h>
#include <QuestManager.h>
#include <UIHandler.h>

#include <raymath.h>

namespace Nawia::Entity {

	namespace {
		std::string readStringAlias(const nlohmann::json& data, const std::initializer_list<const char*> keys) {
			for (const char* key : keys) {
				if (data.contains(key) && data[key].is_string())
					return data[key].get<std::string>();
			}
			return "";
		}
	}

	GenericStoryNpc::GenericStoryNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine,
		const nlohmann::json& data)
		: StoryNpc(name, x, y, engine)
	{
		_type = EntityType::NPCActor;
		_npc_class_name = data.value("npc_class", "story_human");
		_can_talk = data.value("can_talk", true);
		_disable_interaction_after_talk = data.value("disable_interaction_after_talk", false);
		_route_after_talk = data.value("route_after_talk", false);
		_hide_on_arrival = data.value("hide_on_arrival", false);
		_stop_distance = data.value("stop_distance", 0.65f);
		_destination_name = data.value("destination_name", data.value("target_entity", ""));
		_start_quest_id = data.value("start_quest", "");
		_complete_quest_id = data.value("complete_quest", "");
		_fail_quest_id = data.value("fail_quest", "");
		_checkpoint_on_talk = data.value("checkpoint_on_talk", data.value("checkpoint_on_complete", ""));
		_checkpoint_on_arrival = data.value("checkpoint_on_arrival", "");

		if (data.contains("destination_x") && data.contains("destination_y")) {
			_destination_position = Vector2{
				data.value("destination_x", 0.0f),
				data.value("destination_y", 0.0f)
			};
		}

		setMovementSpeed(data.value("movement_speed", 2.0f));
		setScale(data.value("scale", 1.55f));
		setModelFacingOffset(data.value("model_facing_offset", 90.0f));
		configureModel(data);
		configureDialogue(data);
		playIdleAnimation();
	}

	void GenericStoryNpc::configureModel(const nlohmann::json& data) {
		_model_path = readStringAlias(data, {"model", "model_path"});
		_animation_bundle_path = data.value("animation_bundle", _model_path);
		_idle_animation = data.value("idle_animation", _idle_animation);
		_walk_animation = data.value("walk_animation", _walk_animation);
		_talk_animation = data.value("talk_animation", _talk_animation);

		if (_model_path.empty()) {
			Core::Logger::errorLog("GenericStoryNpc '" + getName() + "' wymaga pola model/model_path.");
			return;
		}

		loadModel(_model_path, data.value("rotate_model", false));
		if (!_animation_bundle_path.empty())
			loadAnimationBundle(_animation_bundle_path);

		if (data.value("use_indexed_animation_aliases", false)) {
			addAnimation("death", _model_path, data.value("death_animation_index", 0));
			addAnimation("idle", _model_path, data.value("idle_animation_index", 4));
			addAnimation("walk", _model_path, data.value("walk_animation_index", 22));
			addAnimation("walk_back", _model_path, data.value("walk_back_animation_index", 17));
		}

		const float target_height = data.value("target_height", 0.0f);
		if (target_height > 0.0f)
			fitLoadedModelToHeight(target_height);
		else
			setScale(data.value("scale", getScale()));
	}

	void GenericStoryNpc::configureDialogue(const nlohmann::json& data) {
		_dialogue_key = data.value("dialogue_key", "");
		if (!_dialogue_key.empty()) {
			setDialogueStageKey(_dialogue_key);
			setDialogue(buildDialogueFromConfig(_dialogue_key));
			return;
		}

		setDialogueStageKey(_npc_class_name);
		setPlaceholderDialogue(getName(), data.value("placeholder_text", "..."));
	}

	void GenericStoryNpc::onInteract(Entity& instigator) {
		rotateTowardsCenter(instigator.getCenter().x, instigator.getCenter().y);
		instigator.rotateTowardsCenter(getCenter().x, getCenter().y);
		playIdleAnimation();
	}

	bool GenericStoryNpc::canInteract() const {
		if (!_can_talk)
			return false;

		if (_disable_interaction_after_talk && _talk_completed)
			return false;

		return !_walking_to_destination && !_arrived;
	}

	void GenericStoryNpc::update(const float delta_time) {
		if (isDormant())
			return;

		if (_walking_to_destination) {
			Entity::update(delta_time);
			if (!isAnimationLocked())
				updateRouteToDestination(delta_time);
			return;
		}

		StoryNpc::update(delta_time);
	}

	void GenericStoryNpc::handleQuestTalkCompleted(Core::Engine& engine) {
		if (_talk_completed && _disable_interaction_after_talk)
			return;

		_talk_completed = true;
		executeTalkActions(engine);

		if (_route_after_talk)
			startRoute(engine);
	}

	void GenericStoryNpc::executeTalkActions(Core::Engine& engine) {
		if (!_checkpoint_on_talk.empty()) {
			engine.getQuestManager().notifyCheckpointReached(_checkpoint_on_talk);
			engine.getQuestManager().update(&engine);
		}

		if (!_start_quest_id.empty())
			engine.getQuestManager().startQuest(_start_quest_id);

		if (!_complete_quest_id.empty())
			engine.getQuestManager().completeQuest(_complete_quest_id, &engine);

		if (!_fail_quest_id.empty())
			engine.getQuestManager().failQuest(_fail_quest_id, &engine);
	}

	void GenericStoryNpc::startRoute(Core::Engine& engine) {
		const auto destination = resolveDestination(engine);
		if (!destination) {
			Core::Logger::errorLog("GenericStoryNpc '" + getName() + "' nie znalazl celu trasy.");
			return;
		}

		_destination = *destination;
		_walking_to_destination = true;
		_path_requested = false;
		buildPathToPoint(_destination);
	}

	std::optional<Vector2> GenericStoryNpc::resolveDestination(Core::Engine& engine) const {
		if (_destination_position)
			return _destination_position;

		if (_destination_name.empty())
			return std::nullopt;

		for (const auto& entity : engine.getEntityManager().getEntities()) {
			if (entity && entity->getName() == _destination_name)
				return Vector2{entity->getX(), entity->getY()};
		}

		return std::nullopt;
	}

	void GenericStoryNpc::updateRouteToDestination(const float delta_time) {
		const float distance = Vector2Distance(getCenter(), _destination);
		if (distance <= _stop_distance) {
			stopPathMovement();
			_walking_to_destination = false;
			_arrived = true;
			playIdleAnimation();

			if (_engine && !_checkpoint_on_arrival.empty()) {
				_engine->getQuestManager().notifyCheckpointReached(_checkpoint_on_arrival);
				_engine->getQuestManager().update(_engine);
			}

			if (_hide_on_arrival)
				setDormant(true);
			return;
		}

		if (!_path_requested) {
			buildPathToPoint(_destination);
			_path_requested = true;
		}

		updatePathMovement(delta_time);
		if (_is_moving)
			playWalkAnimation();
	}

	void GenericStoryNpc::buildPathToPoint(const Vector2 target) {
		_current_path.clear();

		if (_engine && _engine->getCurrentMap() && _engine->getCurrentMap()->getNavMesh().isReady())
			_current_path = _engine->getCurrentMap()->findPath(getWorldPos3D(), {target.x, getAltitude(), target.y});

		if (_current_path.empty())
			_current_path.push_back(target);

		trimCurrentPathStart();

		if (!_current_path.empty())
			moveTo(_current_path.front().x, _current_path.front().y);
		else
			stopPathMovement();
	}

	void GenericStoryNpc::trimCurrentPathStart() {
		if (_current_path.empty())
			return;

		const Vector2 first_path_point = _current_path.front();
		const float dx = first_path_point.x - getCenter().x;
		const float dy = first_path_point.y - getCenter().y;
		if (dx * dx + dy * dy < 0.1f)
			_current_path.erase(_current_path.begin());
	}

	void GenericStoryNpc::updatePathMovement(const float delta_time) {
		if (!_is_moving && !_current_path.empty()) {
			_current_path.erase(_current_path.begin());

			if (!_current_path.empty())
				moveTo(_current_path.front().x, _current_path.front().y);
		}

		updateMovement(delta_time);
	}

	void GenericStoryNpc::stopPathMovement() {
		_current_path.clear();
		_is_moving = false;
		setVelocity(0.0f, 0.0f);
	}

	void GenericStoryNpc::playIdleAnimation() {
		if (getAnimationFrameCount(_idle_animation) > 0)
			playAnimation(_idle_animation, true, false, 0, true);
		else if (getAnimationFrameCount("idle") > 0)
			playAnimation("idle", true, false, 0, true);
		else if (getAnimationFrameCount("Idle") > 0)
			playAnimation("Idle", true, false, 0, true);
		else if (getAnimationFrameCount("Idle_Loop") > 0)
			playAnimation("Idle_Loop", true, false, 0, true);
		else if (getAnimationFrameCount("default") > 0)
			playAnimation("default", true, false, 0, true);
	}

	void GenericStoryNpc::playWalkAnimation() {
		if (getAnimationFrameCount(_walk_animation) > 0)
			playAnimation(_walk_animation);
		else if (getAnimationFrameCount("walk") > 0)
			playAnimation("walk");
		else if (getAnimationFrameCount("Walk") > 0)
			playAnimation("Walk");
		else if (getAnimationFrameCount("Walk_Loop") > 0)
			playAnimation("Walk_Loop");
	}

	nlohmann::json GenericStoryNpc::serializeState() const {
		nlohmann::json state = StoryNpc::serializeState();
		state["talk_completed"] = _talk_completed;
		state["walking_to_destination"] = _walking_to_destination;
		state["arrived"] = _arrived;
		state["destination"] = {{"x", _destination.x}, {"y", _destination.y}};
		return state;
	}

	void GenericStoryNpc::applyState(const nlohmann::json& state, Item::ItemDatabase* item_database) {
		StoryNpc::applyState(state, item_database);
		if (!state.is_object())
			return;

		_talk_completed = state.value("talk_completed", _talk_completed);
		_walking_to_destination = state.value("walking_to_destination", _walking_to_destination);
		_arrived = state.value("arrived", _arrived);
		if (state.contains("destination") && state["destination"].is_object()) {
			_destination = {
				state["destination"].value("x", _destination.x),
				state["destination"].value("y", _destination.y)
			};
		}

		if (_walking_to_destination)
			buildPathToPoint(_destination);
	}

} // namespace Nawia::Entity
