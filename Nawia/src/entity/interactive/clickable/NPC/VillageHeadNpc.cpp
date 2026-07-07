#include "VillageHeadNpc.h"

#include <Engine.h>
#include <EntityManager.h>
#include <EntityPathMotion.h>
#include <Map.h>

#include <raymath.h>

namespace Nawia::Entity {

	namespace {
		constexpr const char* VILLAGE_HEAD_MODEL = "assets/models/actors/village_head/village_head.glb";
		constexpr float VILLAGE_HEAD_SCALE = 1.55f;
		constexpr float VILLAGE_HEAD_STOP_DISTANCE = 0.65f;
	}

	VillageHeadNpc::VillageHeadNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine)
		: StoryNpc(name, x, y, engine)
	{
		setType(EntityType::NPCActor);
		setScale(VILLAGE_HEAD_SCALE);
		setMovementSpeed(2.0f);
		loadModel(VILLAGE_HEAD_MODEL);
		loadAnimationBundle(VILLAGE_HEAD_MODEL);
		playAnimation("Idle", true, false, 0, true);
		setDialogueStageKey("village_head");
		setDialogue(buildDialogueFromConfig("village_head"));
	}

	void VillageHeadNpc::onInteract(Entity& instigator) {
		rotateTowardsCenter(instigator.getCenter().x, instigator.getCenter().y);
		instigator.rotateTowardsCenter(getCenter().x, getCenter().y);

		if (getAnimationFrameCount("Idle") > 0)
			playAnimation("Idle", true, false, 0, true);
	}

	bool VillageHeadNpc::isMouseOver(const float screen_x, const float screen_y, const Camera3D& camera) const {
		return !_survivor_quest_started && StoryNpc::isMouseOver(screen_x, screen_y, camera);
	}

	bool VillageHeadNpc::canInteract() const {
		return !_survivor_quest_started;
	}

	void VillageHeadNpc::update(const float delta_time) {
		if (isDormant())
			return;

		Entity::update(delta_time);
		if (isAnimationLocked())
			return;

		updateRouteToDestination(delta_time);
	}

	void VillageHeadNpc::handleQuestTalkCompleted(Core::Engine& engine) {
		if (_survivor_quest_started)
			return;

		engine.getQuestManager().update(&engine);
		_survivor_quest_started = true;
		bool started_survivor_quest = engine.getQuestManager().startQuest("rescue_cemetery_survivors");
		started_survivor_quest = engine.getQuestManager().startQuest("rescue_forest_survivors") || started_survivor_quest;
		started_survivor_quest = engine.getQuestManager().startQuest("enter_nawia_threshold") || started_survivor_quest;
		const auto* cemetery_quest = engine.getQuestManager().getQuest("rescue_cemetery_survivors");
		const auto* forest_quest = engine.getQuestManager().getQuest("rescue_forest_survivors");
		const auto* witch_quest = engine.getQuestManager().getQuest("enter_nawia_threshold");
		if (started_survivor_quest ||
			(cemetery_quest && cemetery_quest->isActive()) ||
			(forest_quest && forest_quest->isActive()) ||
			(witch_quest && witch_quest->isActive()))
			engine.getUIHandler().showNotification("Nowe questy: ocaleni i Wiedzma", 4.0f);
		startRouteToHerbalistHub();
	}

	void VillageHeadNpc::startRouteToHerbalistHub() {
		if (!_engine)
			return;

		Vector3 destination = {0.0f, getAltitude(), 0.0f};
		bool has_destination = false;
		for (const auto& entity : _engine->getEntityManager().getEntities()) {
			if (entity && entity->getName() == "Herbalist Hub") {
				destination = {entity->getX(), getAltitude(), entity->getY()};
				has_destination = true;
				break;
			}
		}

		if (!has_destination) {
			const auto player = _engine->getPlayer();
			if (!player)
				return;

			destination = {player->getRespawnPoint().x, getAltitude(), player->getRespawnPoint().y};
		}

		if (_engine->getCurrentMap() && _engine->getCurrentMap()->getNavMesh().isReady())
			destination = _engine->getCurrentMap()->getNavMesh().getClosestWalkablePosition({destination.x, 0.0f, destination.z});

		_destination = {destination.x, destination.z};
		_walking_to_spawn = true;
		_path_requested = false;
		buildPathToPoint(_destination);
	}

	void VillageHeadNpc::updateRouteToDestination(const float delta_time) {
		if (!_walking_to_spawn)
			return;

		const float distance = Vector2Distance(getCenter(), _destination);
		if (distance <= VILLAGE_HEAD_STOP_DISTANCE) {
			stopPathMovement();
			_walking_to_spawn = false;
			if (getAnimationFrameCount("Idle") > 0)
				playAnimation("Idle", true, false, 0, true);
			return;
		}

		if (!_path_requested) {
			buildPathToPoint(_destination);
			_path_requested = true;
		}

		updatePathMovement(delta_time);

		if (isMoving()) {
			if (getAnimationFrameCount("Walk") > 0)
				playAnimation("Walk");
			else if (getAnimationFrameCount("walk") > 0)
				playAnimation("walk");
		}
	}

	void VillageHeadNpc::buildPathToPoint(const Vector2 target) {
		PathMotion::buildPathToPoint(
			*this,
			_engine ? _engine->getCurrentMap() : nullptr,
			target,
			_current_path);
	}

	void VillageHeadNpc::updatePathMovement(const float delta_time) {
		PathMotion::updatePathMovement(*this, delta_time, _current_path);
	}

	void VillageHeadNpc::stopPathMovement() {
		PathMotion::stopPathMovement(*this, _current_path);
	}

	nlohmann::json VillageHeadNpc::serializeState() const {
		nlohmann::json state = StoryNpc::serializeState();
		state["survivor_quest_started"] = _survivor_quest_started;
		state["walking_to_spawn"] = _walking_to_spawn;
		state["destination"] = {{"x", _destination.x}, {"y", _destination.y}};
		return state;
	}

	void VillageHeadNpc::applyState(const nlohmann::json& state, Item::ItemDatabase* item_database) {
		StoryNpc::applyState(state, item_database);
		if (!state.is_object())
			return;

		_survivor_quest_started = state.value("survivor_quest_started", _survivor_quest_started);
		_walking_to_spawn = state.value("walking_to_spawn", _walking_to_spawn);
		_path_requested = false;
		if (state.contains("destination") && state["destination"].is_object()) {
			_destination = {
				state["destination"].value("x", _destination.x),
				state["destination"].value("y", _destination.y)
			};
		}

		if (_walking_to_spawn)
			buildPathToPoint(_destination);
		else if (getAnimationFrameCount("Idle") > 0)
			playAnimation("Idle", true, false, 0, true);
	}

} // namespace Nawia::Entity
