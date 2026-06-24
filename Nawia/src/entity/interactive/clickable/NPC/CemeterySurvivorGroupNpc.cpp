#include "CemeterySurvivorGroupNpc.h"

#include <Engine.h>
#include <EntityManager.h>
#include <HerbalistHub.h>
#include <Logger.h>
#include <Map.h>
#include <QuestManager.h>

#include <algorithm>
#include <cmath>
#include <raymath.h>

namespace Nawia::Entity {

	namespace {
		constexpr const char* FEMALE_SURVIVOR_MODEL = "assets/models/actors/friends/female_warrior/female_warrior.glb";
		constexpr const char* MALE_SURVIVOR_MODEL = "assets/models/actors/npcs/male_npc_1.glb";
		constexpr const char* ANIM_IDLE = "cemetery_idle";
		constexpr const char* ANIM_WALK = "cemetery_walk";
		constexpr float SURVIVOR_SCALE = 1.55f;
	}

	class CemeterySurvivorGroupNpc::GroupVisual final : public Entity {
	public:
		GroupVisual(const std::string& name, const float x, const float y)
			: Entity(name, x, y, nullptr, 1)
		{
			_type = EntityType::NPCStatic;
			setFaction(Faction::None);
		}

		bool isMovingToTarget() const { return _is_moving; }
		void updateMoveToTarget(const float delta_time) { updateMovement(delta_time); }
		void updateVisualAnimation(const float delta_time) { updateAnimation(delta_time); }
	};

	CemeterySurvivorGroupNpc::CemeterySurvivorGroupNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine,
		const nlohmann::json& data)
		: StoryNpc(name, x, y, engine)
	{
		_type = EntityType::NPCActor;
		configureFromJson(data);

		loadModelAndAnimations(*this, FEMALE_SURVIVOR_MODEL);
		playIdle(*this);
		initializeMaleSurvivor();
		snapToNavmesh();

		setDialogueStageKey(_dialogue_key);
		setDialogue(buildDialogueFromConfig(_dialogue_key));
		if (getDialogueTree().getNode(0) == nullptr)
			setPlaceholderDialogue("Ocalona", "Dzieki za ratunek. Idziemy do zielarza.");
	}

	CemeterySurvivorGroupNpc::~CemeterySurvivorGroupNpc() = default;

	void CemeterySurvivorGroupNpc::configureFromJson(const nlohmann::json& data) {
		setMovementSpeed(data.value("movement_speed", 2.0f));
		setScale(data.value("scale", SURVIVOR_SCALE));
		setModelFacingOffset(data.value("model_facing_offset", 90.0f));
		_dialogue_key = data.value("dialogue_key", _dialogue_key);
		_hub_name = data.value("hub_name", data.value("destination_name", _hub_name));
		_hub_radius_fallback = data.value("hub_radius", _hub_radius_fallback);
		_stop_distance = data.value("stop_distance", _stop_distance);
		_checkpoint_on_arrival = data.value("checkpoint_on_arrival", _checkpoint_on_arrival);
		_idle_animation_index = data.value("idle_animation_index", _idle_animation_index);
		_walk_animation_index = data.value("walk_animation_index", _walk_animation_index);
	}

	void CemeterySurvivorGroupNpc::initializeMaleSurvivor() {
		_male_survivor = std::make_unique<GroupVisual>("Ocalony z cmentarza", getX() + 1.0f, getY() + 0.35f);
		_male_survivor->setScale(getScale());
		_male_survivor->setModelFacingOffset(getModelFacingOffset());
		loadModelAndAnimations(*_male_survivor, MALE_SURVIVOR_MODEL);
		playIdle(*_male_survivor);
	}

	void CemeterySurvivorGroupNpc::loadModelAndAnimations(Entity& entity, const std::string& model_path) const {
		entity.loadModel(model_path);
		if (model_path == FEMALE_SURVIVOR_MODEL)
			entity.hideMeshIndex(1); // Mesh 1 to node Sword w female_warrior.glb.
		entity.loadAnimationBundle(model_path);
		entity.addAnimation(ANIM_IDLE, model_path, _idle_animation_index);
		entity.addAnimation(ANIM_WALK, model_path, _walk_animation_index);
	}

	void CemeterySurvivorGroupNpc::playIdle(Entity& entity) const {
		entity.setAnimationSpeed(1.0f);
		if (entity.getAnimationFrameCount(ANIM_IDLE) > 0)
			entity.playAnimation(ANIM_IDLE, true, false, 0, true);
		else if (entity.getAnimationFrameCount("Idle") > 0)
			entity.playAnimation("Idle", true, false, 0, true);
	}

	void CemeterySurvivorGroupNpc::playWalk(Entity& entity) const {
		entity.setAnimationSpeed(1.0f);
		if (entity.getAnimationFrameCount(ANIM_WALK) > 0)
			entity.playAnimation(ANIM_WALK);
		else if (entity.getAnimationFrameCount("Walk") > 0)
			entity.playAnimation("Walk");
		else
			playIdle(entity);
	}

	void CemeterySurvivorGroupNpc::snapToNavmesh() {
		if (!_engine || !_engine->getCurrentMap() || !_engine->getCurrentMap()->getNavMesh().isReady())
			return;

		const Vector3 snapped = _engine->getCurrentMap()->getNavMesh().getClosestWalkablePosition(getWorldPos3D());
		setX(snapped.x);
		setY(snapped.z);
		setAltitude(snapped.y);
		if (_male_survivor) {
			const Vector3 male_snapped = _engine->getCurrentMap()->getNavMesh().getClosestWalkablePosition(
				{_male_survivor->getX(), snapped.y, _male_survivor->getY()});
			_male_survivor->setX(male_snapped.x);
			_male_survivor->setY(male_snapped.z);
			_male_survivor->setAltitude(male_snapped.y);
		}
	}

	void CemeterySurvivorGroupNpc::onInteract(Entity& instigator) {
		if (!canInteract())
			return;

		rotateTowardsCenter(instigator.getCenter().x, instigator.getCenter().y);
		instigator.rotateTowardsCenter(getCenter().x, getCenter().y);
		playIdle(*this);
	}

	bool CemeterySurvivorGroupNpc::canInteract() const {
		return !_talk_completed && !_walking_to_hub && !_dispersing && !_arrived;
	}

	void CemeterySurvivorGroupNpc::handleQuestTalkCompleted(Core::Engine& engine) {
		if (_talk_completed)
			return;

		_talk_completed = true;
		startRoute(engine);
	}

	void CemeterySurvivorGroupNpc::startRoute(Core::Engine& engine) {
		const auto hub = resolveHub(engine);
		_destination = hub ? hub->center : Vector2{getX() + 8.0f, getY()};
		_walking_to_hub = true;
		_path_requested = false;
		buildPathToPoint(_destination);
	}

	std::optional<CemeterySurvivorGroupNpc::HubDestination> CemeterySurvivorGroupNpc::resolveHub(Core::Engine& engine) const {
		for (const auto& entity : engine.getEntityManager().getEntities()) {
			if (!entity || entity->getName() != _hub_name)
				continue;

			HubDestination hub;
			hub.center = {entity->getX(), entity->getY()};
			hub.radius = _hub_radius_fallback;
			if (const auto herbalist_hub = dynamic_cast<HerbalistHub*>(entity.get()))
				hub.radius = herbalist_hub->getRadius();
			return hub;
		}

		return std::nullopt;
	}

	void CemeterySurvivorGroupNpc::update(const float delta_time) {
		if (isDormant())
			return;

		if (_walking_to_hub)
			updateRoute(delta_time);
		else if (_dispersing)
			updateDispersal(delta_time);
		else
			StoryNpc::update(delta_time);

		if (_male_survivor)
			_male_survivor->updateVisualAnimation(delta_time);
	}

	void CemeterySurvivorGroupNpc::render(const Camera3D& camera) {
		if (!canInteract())
			setHovered(false);

		Entity::render(camera);
		if (_male_survivor)
			_male_survivor->render(camera);
	}

	void CemeterySurvivorGroupNpc::updateRoute(const float delta_time) {
		updateAnimation(delta_time);
		const float distance = Vector2Distance(getCenter(), _destination);
		if (distance <= _stop_distance) {
			HubDestination hub;
			if (_engine) {
				if (const auto resolved = resolveHub(*_engine))
					hub = *resolved;
				else
					hub.center = _destination;
			}
			hub.radius = std::max(0.1f, hub.radius);
			stopPathMovement();
			_walking_to_hub = false;
			startDispersal(hub);
			return;
		}

		if (!_path_requested) {
			buildPathToPoint(_destination);
			_path_requested = true;
		}

		updatePathMovement(delta_time);
		if (_is_moving)
			playWalk(*this);

		if (_male_survivor) {
			_male_survivor->setAltitude(getAltitude());
			_male_survivor->moveTo(getX() + 0.9f, getY() + 0.45f);
			_male_survivor->updateMoveToTarget(delta_time);
			playWalk(*_male_survivor);
		}
	}

	void CemeterySurvivorGroupNpc::buildPathToPoint(const Vector2 target) {
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

	void CemeterySurvivorGroupNpc::trimCurrentPathStart() {
		if (_current_path.empty())
			return;

		const Vector2 first = _current_path.front();
		const float dx = first.x - getCenter().x;
		const float dy = first.y - getCenter().y;
		if (dx * dx + dy * dy < 0.1f)
			_current_path.erase(_current_path.begin());
	}

	void CemeterySurvivorGroupNpc::updatePathMovement(const float delta_time) {
		if (!_is_moving && !_current_path.empty()) {
			_current_path.erase(_current_path.begin());
			if (!_current_path.empty())
				moveTo(_current_path.front().x, _current_path.front().y);
		}
		updateMovement(delta_time);
	}

	Vector2 CemeterySurvivorGroupNpc::randomPointInHub(const HubDestination& hub) const {
		const float angle = static_cast<float>(GetRandomValue(0, 6283)) / 1000.0f;
		const float radius = std::sqrt(static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f) * std::max(0.1f, hub.radius);
		return {hub.center.x + std::cos(angle) * radius, hub.center.y + std::sin(angle) * radius};
	}

	void CemeterySurvivorGroupNpc::startDispersal(const HubDestination& hub) {
		_arrival_hub = hub;
		_dispersing = true;
		const Vector2 female = randomPointInHub(_arrival_hub);
		const Vector2 male = randomPointInHub(_arrival_hub);
		moveTo(female.x, female.y);
		playWalk(*this);
		if (_male_survivor) {
			_male_survivor->moveTo(male.x, male.y);
			playWalk(*_male_survivor);
		}
	}

	void CemeterySurvivorGroupNpc::updateDispersal(const float delta_time) {
		if (_is_moving) {
			updateMovement(delta_time);
			playWalk(*this);
		} else {
			playIdle(*this);
		}

		if (_male_survivor) {
			if (_male_survivor->isMovingToTarget()) {
				_male_survivor->updateMoveToTarget(delta_time);
				playWalk(*_male_survivor);
			} else {
				playIdle(*_male_survivor);
			}
		}

		if (!_is_moving && (!_male_survivor || !_male_survivor->isMovingToTarget()))
			finishArrival();
	}

	void CemeterySurvivorGroupNpc::finishArrival() {
		if (_arrived)
			return;

		_arrived = true;
		_dispersing = false;
		playIdle(*this);
		if (_male_survivor)
			playIdle(*_male_survivor);

		if (_engine && !_checkpoint_on_arrival.empty()) {
			_engine->getQuestManager().notifyCheckpointReached(_checkpoint_on_arrival);
			_engine->getQuestManager().update(_engine);
		}
	}

	void CemeterySurvivorGroupNpc::stopPathMovement() {
		_current_path.clear();
		_is_moving = false;
		setVelocity(0.0f, 0.0f);
	}

	nlohmann::json CemeterySurvivorGroupNpc::serializeState() const {
		nlohmann::json state = StoryNpc::serializeState();
		state["talk_completed"] = _talk_completed;
		state["walking_to_hub"] = _walking_to_hub;
		state["dispersing"] = _dispersing;
		state["arrived"] = _arrived;
		state["path_requested"] = _path_requested;
		state["destination"] = {{"x", _destination.x}, {"y", _destination.y}};
		state["arrival_hub"] = {
			{"x", _arrival_hub.center.x},
			{"y", _arrival_hub.center.y},
			{"radius", _arrival_hub.radius}
		};
		if (_male_survivor)
			state["male_survivor"] = _male_survivor->serializeState();
		return state;
	}

	void CemeterySurvivorGroupNpc::applyState(const nlohmann::json& state, Item::ItemDatabase* item_database) {
		StoryNpc::applyState(state, item_database);
		if (!state.is_object())
			return;

		_talk_completed = state.value("talk_completed", _talk_completed);
		_walking_to_hub = state.value("walking_to_hub", _walking_to_hub);
		_dispersing = state.value("dispersing", _dispersing);
		_arrived = state.value("arrived", _arrived);
		_path_requested = state.value("path_requested", false);
		if (state.contains("destination") && state["destination"].is_object()) {
			_destination = {
				state["destination"].value("x", _destination.x),
				state["destination"].value("y", _destination.y)
			};
		}
		if (state.contains("arrival_hub") && state["arrival_hub"].is_object()) {
			_arrival_hub.center = {
				state["arrival_hub"].value("x", _arrival_hub.center.x),
				state["arrival_hub"].value("y", _arrival_hub.center.y)
			};
			_arrival_hub.radius = state["arrival_hub"].value("radius", _arrival_hub.radius);
		}
		if (_male_survivor && state.contains("male_survivor"))
			_male_survivor->applyState(state["male_survivor"], item_database);

		if (_arrived) {
			_walking_to_hub = false;
			_dispersing = false;
			playIdle(*this);
			if (_male_survivor) {
				if (!state.contains("male_survivor")) {
					_male_survivor->setX(getX() + 0.9f);
					_male_survivor->setY(getY() + 0.45f);
					_male_survivor->setAltitude(getAltitude());
				}
				playIdle(*_male_survivor);
			}
		} else if (_walking_to_hub) {
			buildPathToPoint(_destination);
			playWalk(*this);
			if (_male_survivor)
				playWalk(*_male_survivor);
		} else if (_dispersing) {
			if (_arrival_hub.radius <= 0.0f) {
				if (_engine)
					_arrival_hub = resolveHub(*_engine).value_or(HubDestination{_destination, _hub_radius_fallback});
				else
					_arrival_hub = HubDestination{_destination, _hub_radius_fallback};
			}
			startDispersal(_arrival_hub);
		} else {
			playIdle(*this);
			if (_male_survivor)
				playIdle(*_male_survivor);
		}
	}

} // namespace Nawia::Entity
