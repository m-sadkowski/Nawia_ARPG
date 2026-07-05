#include "ForestLostGroupNpc.h"

#include <Engine.h>
#include <EntityManager.h>
#include <HerbalistHub.h>
#include <Logger.h>
#include <Map.h>
#include <Player.h>
#include <QuestManager.h>
#include <UIHandler.h>

#include <algorithm>
#include <cmath>
#include <raymath.h>

namespace Nawia::Entity {

	namespace {
		constexpr const char* FEMALE_CARRIER_MODEL = "assets/models/actors/npcs/female_npc_2.glb";
		constexpr const char* MALE_CARRIER_MODEL = "assets/models/actors/npcs/male_npc_2.glb";
		constexpr const char* MILENA_SISTER_MODEL = "assets/models/actors/milena_sister/milena_sister.glb";

		constexpr const char* ANIM_DEATH = "forest_death";
		constexpr const char* ANIM_IDLE = "forest_idle";
		constexpr const char* ANIM_WALK = "forest_walk";
		constexpr const char* ANIM_WALK_BACK = "forest_walk_back";
		constexpr const char* MILENA_SISTER_NAME = "Siostra Mileny";
		constexpr int MILENA_SCARF_ITEM_ID = 14;
		constexpr float FOREST_NPC_SCALE = 1.55f;

		Vector2 normalizedOrFallback(const Vector2 vector, const Vector2 fallback) {
			if (Vector2LengthSqr(vector) <= 0.0001f)
				return fallback;
			return Vector2Normalize(vector);
		}

		bool removeItemFromBackpack(Player& player, const int item_id) {
			auto& backpack = player.getBackpack();
			for (int i = 0; i < backpack.getCapacity(); ++i) {
				const auto item = backpack.getItem(i);
				if (item && item->getId() == item_id) {
					backpack.removeItem(i);
					return true;
				}
			}

			return false;
		}
	}

	class ForestLostGroupNpc::ForestGroupVisual final : public Entity {
	public:
		ForestGroupVisual(const std::string& name, const float x, const float y)
			: Entity(name, x, y, nullptr, 1)
		{
			setType(EntityType::NPCStatic);
			setFaction(Faction::None);
		}

		bool isMovingToTarget() const {
			return isMoving();
		}

		void updateMoveToTarget(const float delta_time) {
			updateMovement(delta_time);
		}

		void updateVisualAnimation(const float delta_time) {
			updateAnimation(delta_time);
		}

		void holdAnimationFrame(const std::string& animation_name, const int frame) {
			Entity::holdAnimationFrame(animation_name, frame);
		}

		void playAnimationReverseOnce(const std::string& animation_name) {
			Entity::playAnimationReverseOnce(animation_name);
		}
	};

	ForestLostGroupNpc::ForestLostGroupNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine,
		const nlohmann::json& data)
		: StoryNpc(name, x, y, engine)
	{
		setType(EntityType::NPCActor);
		configureFromJson(data);

		loadGroupModelAndAnimations(*this, FEMALE_CARRIER_MODEL);
		playIdle(*this);

		initializeMembers();
		snapGroupToNavmesh();

		setDialogueStageKey(_dialogue_key);
		if (!_dialogue_key.empty())
			setDialogue(buildDialogueFromConfig(_dialogue_key));
		if (getDialogueTree().getNode(0) == nullptr)
			setPlaceholderDialogue("Ocalona", "Musimy zaniesc ja do zielarza. Szybko.");
	}

	ForestLostGroupNpc::~ForestLostGroupNpc() = default;

	void ForestLostGroupNpc::configureFromJson(const nlohmann::json& data) {
		setMovementSpeed(data.value("movement_speed", 1.8f));
		setScale(data.value("scale", FOREST_NPC_SCALE));
		setModelFacingOffset(data.value("model_facing_offset", 90.0f));
		_dialogue_key = data.value("dialogue_key", _dialogue_key);
		_hub_name = data.value("hub_name", data.value("destination_name", _hub_name));
		_hub_radius_fallback = data.value("hub_radius", _hub_radius_fallback);
		_stop_distance = data.value("stop_distance", _stop_distance);
		_tuning.spacing = data.value("carry_spacing", _tuning.spacing);
		_tuning.sister_bob_height = data.value("sister_bob_height", _tuning.sister_bob_height);
		_tuning.sister_carry_height = data.value("sister_carry_height", _tuning.sister_carry_height);
		_tuning.sister_drop_duration = data.value("sister_drop_duration", _tuning.sister_drop_duration);
		_tuning.male_spacing_multiplier = data.value("male_carry_spacing_multiplier", _tuning.male_spacing_multiplier);
		_animation_indices.death = data.value("death_animation_index", _animation_indices.death);
		_animation_indices.idle = data.value("idle_animation_index", _animation_indices.idle);
		_animation_indices.walk = data.value("walk_animation_index", _animation_indices.walk);
		_animation_indices.walk_back = data.value("walk_back_animation_index", _animation_indices.walk_back);
		_checkpoint_on_arrival = data.value("checkpoint_on_arrival", data.value("checkpoint_on_complete", ""));
		_start_quest_id = data.value("start_quest", "");
		_complete_quest_id = data.value("complete_quest", "");
	}

	void ForestLostGroupNpc::initializeMembers() {
		_male_carrier = std::make_unique<ForestGroupVisual>("male_npc_2", getX(), getY());
		_male_carrier->setScale(getScale());
		_male_carrier->setModelFacingOffset(getModelFacingOffset());
		loadGroupModelAndAnimations(*_male_carrier, MALE_CARRIER_MODEL);
		playIdle(*_male_carrier);

		_milena_sister = std::make_unique<ForestGroupVisual>("milena_sister", getX(), getY());
		_milena_sister->setScale(getScale());
		_milena_sister->setModelFacingOffset(getModelFacingOffset());
		loadGroupModelAndAnimations(*_milena_sister, MILENA_SISTER_MODEL);
		// Siostra zaczyna jako bezwladne cialo niesione przez dwoje NPC.
		freezeSisterOnDeathFrame();

		snapMembersToFormation(_last_travel_direction);
	}

	void ForestLostGroupNpc::snapGroupToNavmesh() {
		if (!_engine || !_engine->getCurrentMap() || !_engine->getCurrentMap()->getNavMesh().isReady())
			return;

		const Vector3 snapped = _engine->getCurrentMap()->getNavMesh().getClosestWalkablePosition(getWorldPos3D());
		setX(snapped.x);
		setY(snapped.z);
		setAltitude(snapped.y);
		snapMembersToFormation(_last_travel_direction);
	}

	void ForestLostGroupNpc::loadGroupModelAndAnimations(Entity& entity, const std::string& model_path) const {
		entity.loadModel(model_path);
		entity.loadAnimationBundle(model_path);
		entity.addAnimation(ANIM_DEATH, model_path, _animation_indices.death);
		entity.addAnimation(ANIM_IDLE, model_path, _animation_indices.idle);
		entity.addAnimation(ANIM_WALK, model_path, _animation_indices.walk);
		entity.addAnimation(ANIM_WALK_BACK, model_path, _animation_indices.walk_back);
	}

	void ForestLostGroupNpc::freezeSisterOnDeathFrame() {
		if (!_milena_sister)
			return;

		const int frame_count = _milena_sister->getAnimationFrameCount(ANIM_DEATH);
		if (frame_count > 0)
			_milena_sister->holdAnimationFrame(ANIM_DEATH, frame_count - 1);
		else
			playIdle(*_milena_sister);
	}

	void ForestLostGroupNpc::playIdle(Entity& entity) const {
		entity.setAnimationSpeed(1.0f);
		if (entity.getAnimationFrameCount(ANIM_IDLE) > 0)
			entity.playAnimation(ANIM_IDLE, true, false, 0, true);
		else if (entity.getAnimationFrameCount("Idle") > 0)
			entity.playAnimation("Idle", true, false, 0, true);
		else if (entity.getAnimationFrameCount("idle") > 0)
			entity.playAnimation("idle", true, false, 0, true);
		else if (entity.getAnimationFrameCount("default") > 0)
			entity.playAnimation("default", true, false, 0, true);
	}

	void ForestLostGroupNpc::playWalk(Entity& entity) const {
		entity.setAnimationSpeed(1.0f);
		if (entity.getAnimationFrameCount(ANIM_WALK) > 0)
			entity.playAnimation(ANIM_WALK);
		else if (entity.getAnimationFrameCount("Walk") > 0)
			entity.playAnimation("Walk");
		else if (entity.getAnimationFrameCount("walk") > 0)
			entity.playAnimation("walk");
		else
			playIdle(entity);
	}

	void ForestLostGroupNpc::playWalkBack(Entity& entity) const {
		entity.setAnimationSpeed(1.0f);
		if (entity.getAnimationFrameCount(ANIM_WALK_BACK) > 0)
			entity.playAnimation(ANIM_WALK_BACK);
		else if (entity.getAnimationFrameCount("Run_Back") > 0)
			entity.playAnimation("Run_Back");
		else if (entity.getAnimationFrameCount("Walk_Back") > 0)
			entity.playAnimation("Walk_Back");
		else
			playWalk(entity);
	}

	void ForestLostGroupNpc::onInteract(Entity& instigator) {
		if (_state != CarryState::Waiting && _state != CarryState::Arrived)
			return;

		if (_state == CarryState::Arrived)
			setDialogue(buildDialogueFromConfig("milena_sister_after_herbalist"));

		rotateTowardsCenter(instigator.getCenter().x, instigator.getCenter().y);
		instigator.rotateTowardsCenter(getCenter().x, getCenter().y);
		playIdle(*this);
	}

	bool ForestLostGroupNpc::canInteract() const {
		if (_state == CarryState::Waiting)
			return !_talk_completed;

		if (_state == CarryState::Arrived && _engine) {
			const auto* quest = _engine->getQuestManager().getQuest("talk_to_milena_sister");
			return quest && quest->isActive();
		}

		return false;
	}

	void ForestLostGroupNpc::handleQuestTalkCompleted(Core::Engine& engine) {
		if (_state == CarryState::Arrived) {
			if (const auto player = engine.getPlayer()) {
				if (removeItemFromBackpack(*player, MILENA_SCARF_ITEM_ID))
					engine.getUIHandler().showNotification("Oddano: Jej chusta", 3.0f);
			}
			engine.getQuestManager().update(&engine);
			return;
		}

		if (_talk_completed)
			return;

		_talk_completed = true;
		if (!_start_quest_id.empty())
			engine.getQuestManager().startQuest(_start_quest_id);

		startCarryRoute(engine);
	}

	void ForestLostGroupNpc::startCarryRoute(Core::Engine& engine) {
		const auto hub = resolveHub(engine);
		if (hub)
			_destination = hub->center;
		else
			_destination = Vector2{getX() + 8.0f, getY()};

		_last_travel_direction = normalizedOrFallback(
			Vector2Subtract(_destination, getCenter()),
			_last_travel_direction
		);
		_state = CarryState::Carrying;
		_path_requested = false;
		buildPathToPoint(_destination);
	}

	std::optional<ForestLostGroupNpc::HubDestination> ForestLostGroupNpc::resolveHub(Core::Engine& engine) const {
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

	void ForestLostGroupNpc::update(const float delta_time) {
		if (isDormant())
			return;

		switch (_state) {
		case CarryState::Waiting:
		case CarryState::Arrived:
			StoryNpc::update(delta_time);
			break;
		case CarryState::Carrying:
			updateCarrying(delta_time);
			break;
		case CarryState::SisterDropping:
			updateAnimation(delta_time);
			updateSisterDrop(delta_time);
			break;
		case CarryState::SisterStandingUp:
			updateAnimation(delta_time);
			finishSisterStandUpIfReady();
			break;
		case CarryState::Dispersing:
			updateAnimation(delta_time);
			updateDispersal(delta_time);
			break;
		}

		updateMemberAnimations(delta_time);
	}

	void ForestLostGroupNpc::updateMemberAnimations(const float delta_time) {
		if (_male_carrier)
			_male_carrier->updateVisualAnimation(delta_time);
		if (_milena_sister)
			_milena_sister->updateVisualAnimation(delta_time);
	}

	void ForestLostGroupNpc::render(const Camera3D& camera) {
		// Po zakonczeniu trasy NPC nie powinien podswietlac sie na hover.
		if (!canInteract())
			setHovered(false);

		Entity::render(camera);
		if (_milena_sister && !_main_is_milena_sister)
			_milena_sister->render(camera);
		if (_male_carrier)
			_male_carrier->render(camera);
	}

	void ForestLostGroupNpc::updateCarryMovement(const float delta_time) {
		const float distance = Vector2Distance(getCenter(), _destination);
		if (distance <= _stop_distance) {
			HubDestination hub;
			if (_engine) {
				if (const auto resolved = resolveHub(*_engine))
					hub = *resolved;
				else
					hub.center = _destination;
			} else {
				hub.center = _destination;
			}
			hub.radius = std::max(0.1f, hub.radius);

			stopPathMovement();
			startSisterDrop(hub);
			return;
		}

		if (!_path_requested) {
			buildPathToPoint(_destination);
			_path_requested = true;
		}

		updatePathMovement(delta_time);
		if (isMoving()) {
			playWalkBack(*this);
			if (_male_carrier)
				playWalk(*_male_carrier);
		}
	}

	void ForestLostGroupNpc::buildPathToPoint(const Vector2 target) {
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

	void ForestLostGroupNpc::trimCurrentPathStart() {
		if (_current_path.empty())
			return;

		const Vector2 first_path_point = _current_path.front();
		const float dx = first_path_point.x - getCenter().x;
		const float dy = first_path_point.y - getCenter().y;
		if (dx * dx + dy * dy < 0.1f)
			_current_path.erase(_current_path.begin());
	}

	void ForestLostGroupNpc::updatePathMovement(const float delta_time) {
		if (!isMoving() && !_current_path.empty()) {
			_current_path.erase(_current_path.begin());

			if (!_current_path.empty())
				moveTo(_current_path.front().x, _current_path.front().y);
		}

		const Vector2 before = getCenter();
		updateMovement(delta_time);
		const Vector2 after = getCenter();
		const Vector2 delta = Vector2Subtract(after, before);
		if (Vector2LengthSqr(delta) > 0.0001f)
			_last_travel_direction = Vector2Normalize(delta);
	}

	void ForestLostGroupNpc::stopPathMovement() {
		_current_path.clear();
		stopMovement();
		setVelocity(0.0f, 0.0f);
	}

	void ForestLostGroupNpc::updateCarrying(const float delta_time) {
		updateAnimation(delta_time);
		updateCarryMovement(delta_time);
		if (_state == CarryState::Carrying)
			updateCarryFormation(delta_time);
	}

	void ForestLostGroupNpc::updateCarryFormation(const float delta_time) {
		_sister_bob_time += delta_time;
		const Vector2 direction = currentTravelDirection();
		snapMembersToFormation(direction);

		faceAlongDirection(*this, Vector2Scale(direction, -1.0f));
		if (_male_carrier)
			faceAlongDirection(*_male_carrier, direction);
		if (_milena_sister)
			faceAlongDirection(*_milena_sister, direction);
	}

	void ForestLostGroupNpc::snapMembersToFormation(const Vector2 direction) {
		const Vector2 travel_direction = normalizedOrFallback(direction, {1.0f, 0.0f});
		const Vector2 sister_position = Vector2Subtract(getCenter(), Vector2Scale(travel_direction, _tuning.spacing));
		// Tylny noszacy jest dalej od punktu siostry, zeby wizualnie trzymal okolice glowy.
		const Vector2 male_position = Vector2Subtract(
			getCenter(),
			Vector2Scale(travel_direction, _tuning.spacing * _tuning.male_spacing_multiplier * 2)
		);

		if (_milena_sister) {
			_milena_sister->setX(sister_position.x);
			_milena_sister->setY(sister_position.y);
			const float bob = std::sin(_sister_bob_time * 5.0f) * _tuning.sister_bob_height;
			_milena_sister->setAltitude(getAltitude() + _tuning.sister_carry_height + bob);
		}

		if (_male_carrier) {
			_male_carrier->setX(male_position.x);
			_male_carrier->setY(male_position.y);
			_male_carrier->setAltitude(getAltitude());
		}
	}

	void ForestLostGroupNpc::faceAlongDirection(Entity& entity, const Vector2 direction) const {
		entity.rotateTowards(entity.getX() + direction.x, entity.getY() + direction.y);
	}

	Vector2 ForestLostGroupNpc::currentTravelDirection() const {
		return normalizedOrFallback(_last_travel_direction, {1.0f, 0.0f});
	}

	void ForestLostGroupNpc::startSisterDrop(const HubDestination& hub) {
		_arrival_hub = hub;
		_state = CarryState::SisterDropping;
		_sister_drop_timer = 0.0f;
		_sister_drop_start_altitude = _milena_sister ? _milena_sister->getAltitude() : getAltitude();
		playIdle(*this);

		if (_male_carrier)
			playIdle(*_male_carrier);

		if (_milena_sister)
			freezeSisterOnDeathFrame();
		else
			startDispersal();
	}

	void ForestLostGroupNpc::updateSisterDrop(const float delta_time) {
		if (!_milena_sister) {
			startDispersal();
			return;
		}

		_sister_drop_timer += delta_time;
		const float duration = std::max(0.01f, _tuning.sister_drop_duration);
		const float t = std::clamp(_sister_drop_timer / duration, 0.0f, 1.0f);
		const float eased = t * t * (3.0f - 2.0f * t);
		const float altitude = _sister_drop_start_altitude + (getAltitude() - _sister_drop_start_altitude) * eased;
		_milena_sister->setAltitude(altitude);

		if (t >= 1.0f) {
			_milena_sister->setAltitude(getAltitude());
			startSisterStandUp();
		}
	}

	Vector2 ForestLostGroupNpc::randomPointInHub(const HubDestination& hub) const {
		const float angle = static_cast<float>(GetRandomValue(0, 6283)) / 1000.0f;
		const float radius = std::sqrt(static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f) * std::max(0.1f, hub.radius);
		return {
			hub.center.x + std::cos(angle) * radius,
			hub.center.y + std::sin(angle) * radius
		};
	}

	void ForestLostGroupNpc::startSisterStandUp() {
		_state = CarryState::SisterStandingUp;
		_sister_is_standing = false;
		if (!_milena_sister) {
			startDispersal();
			return;
		}

		_milena_sister->playAnimationReverseOnce(ANIM_DEATH);
	}

	void ForestLostGroupNpc::finishSisterStandUpIfReady() {
		if (!_milena_sister || _sister_is_standing)
			return;

		if (_milena_sister->isAnimationLocked())
			return;

		_sister_is_standing = true;
		playIdle(*_milena_sister);
		startDispersal();
	}

	void ForestLostGroupNpc::startDispersal() {
		_state = CarryState::Dispersing;

		const Vector2 female = randomPointInHub(_arrival_hub);
		const Vector2 male = randomPointInHub(_arrival_hub);
		const Vector2 sister = randomPointInHub(_arrival_hub);

		// Po wstaniu grupa rozchodzi sie jak mini mushroomy: losuje miejsca w hubie i dochodzi do nich ruchem.
		moveTo(female.x, female.y);
		playWalk(*this);

		if (_male_carrier) {
			_male_carrier->setAltitude(getAltitude());
			_male_carrier->moveTo(male.x, male.y);
			playWalk(*_male_carrier);
		}

		if (_milena_sister) {
			_milena_sister->setAltitude(getAltitude());
			_milena_sister->moveTo(sister.x, sister.y);
			playWalk(*_milena_sister);
		}
	}

	void ForestLostGroupNpc::updateDispersal(const float delta_time) {
		if (isMoving()) {
			updateMovement(delta_time);
			playWalk(*this);
		} else {
			playIdle(*this);
		}

		if (_male_carrier) {
			if (_male_carrier->isMovingToTarget()) {
				_male_carrier->updateMoveToTarget(delta_time);
				playWalk(*_male_carrier);
			} else {
				playIdle(*_male_carrier);
			}
		}

		if (_milena_sister) {
			if (_milena_sister->isMovingToTarget()) {
				_milena_sister->updateMoveToTarget(delta_time);
				playWalk(*_milena_sister);
			} else {
				playIdle(*_milena_sister);
			}
		}

		if (!isMoving()
			&& (!_male_carrier || !_male_carrier->isMovingToTarget())
			&& (!_milena_sister || !_milena_sister->isMovingToTarget())) {
			finishArrival();
		}
	}

	void ForestLostGroupNpc::finishArrival() {
		if (_state == CarryState::Arrived)
			return;

		_state = CarryState::Arrived;
		promoteMainEntityToMilenaSister();
		playIdle(*this);
		if (_male_carrier)
			playIdle(*_male_carrier);
		if (_milena_sister) {
			_milena_sister->setAltitude(getAltitude());
			playIdle(*_milena_sister);
		}

		if (_engine && !_checkpoint_on_arrival.empty()) {
			_engine->getQuestManager().notifyCheckpointReached(_checkpoint_on_arrival);
			_engine->getQuestManager().update(_engine);
		}

		if (_engine && !_complete_quest_id.empty())
			_engine->getQuestManager().completeQuest(_complete_quest_id, _engine);
	}

	void ForestLostGroupNpc::promoteMainEntityToMilenaSister(const bool adopt_sister_position) {
		if (_main_is_milena_sister && !_milena_sister)
			return;

		if (_milena_sister && adopt_sister_position) {
			setX(_milena_sister->getX());
			setY(_milena_sister->getY());
			setAltitude(_milena_sister->getAltitude());
		}

		setName(MILENA_SISTER_NAME);
		loadGroupModelAndAnimations(*this, MILENA_SISTER_MODEL);
		playIdle(*this);
		_milena_sister.reset();
		_main_is_milena_sister = true;
	}

	nlohmann::json ForestLostGroupNpc::serializeState() const {
		nlohmann::json state = StoryNpc::serializeState();
		state["talk_completed"] = _talk_completed;
		state["state"] = static_cast<int>(_state);
		state["destination"] = {{"x", _destination.x}, {"y", _destination.y}};
		state["sister_standing"] = _sister_is_standing;
		state["main_is_milena_sister"] = _main_is_milena_sister;
		state["path_requested"] = _path_requested;
		state["sister_bob_time"] = _sister_bob_time;
		state["sister_drop_timer"] = _sister_drop_timer;
		state["sister_drop_start_altitude"] = _sister_drop_start_altitude;
		state["arrival_hub"] = {
			{"x", _arrival_hub.center.x},
			{"y", _arrival_hub.center.y},
			{"radius", _arrival_hub.radius}
		};
		state["last_travel_direction"] = {
			{"x", _last_travel_direction.x},
			{"y", _last_travel_direction.y}
		};
		if (_male_carrier)
			state["male_carrier"] = _male_carrier->serializeState();
		if (_milena_sister)
			state["milena_sister"] = _milena_sister->serializeState();
		return state;
	}

	void ForestLostGroupNpc::applyState(const nlohmann::json& state, Item::ItemDatabase* item_database) {
		StoryNpc::applyState(state, item_database);
		if (!state.is_object())
			return;

		_talk_completed = state.value("talk_completed", _talk_completed);
		const int raw_state = std::clamp(state.value("state", static_cast<int>(_state)), 0, static_cast<int>(CarryState::Arrived));
		_state = static_cast<CarryState>(raw_state);
		_sister_is_standing = state.value("sister_standing", _sister_is_standing);
		const bool saved_main_is_milena_sister = state.value("main_is_milena_sister", _main_is_milena_sister);
		_main_is_milena_sister = false;
		_path_requested = state.value("path_requested", false);
		_sister_bob_time = state.value("sister_bob_time", _sister_bob_time);
		_sister_drop_timer = state.value("sister_drop_timer", _sister_drop_timer);
		_sister_drop_start_altitude = state.value("sister_drop_start_altitude", _sister_drop_start_altitude);
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
		if (state.contains("last_travel_direction") && state["last_travel_direction"].is_object()) {
			_last_travel_direction = normalizedOrFallback({
				state["last_travel_direction"].value("x", _last_travel_direction.x),
				state["last_travel_direction"].value("y", _last_travel_direction.y)
			}, _last_travel_direction);
		}
		if (_male_carrier && state.contains("male_carrier"))
			_male_carrier->applyState(state["male_carrier"], item_database);
		if (_milena_sister && state.contains("milena_sister"))
			_milena_sister->applyState(state["milena_sister"], item_database);

		switch (_state) {
		case CarryState::Waiting:
			playIdle(*this);
			if (_male_carrier)
				playIdle(*_male_carrier);
			if (_milena_sister)
				freezeSisterOnDeathFrame();
			snapMembersToFormation(_last_travel_direction);
			break;
		case CarryState::Carrying:
			buildPathToPoint(_destination);
			playWalkBack(*this);
			if (_male_carrier)
				playWalk(*_male_carrier);
			if (_milena_sister)
				freezeSisterOnDeathFrame();
			snapMembersToFormation(_last_travel_direction);
			break;
		case CarryState::SisterDropping:
			playIdle(*this);
			if (_male_carrier)
				playIdle(*_male_carrier);
			if (_milena_sister)
				freezeSisterOnDeathFrame();
			break;
		case CarryState::SisterStandingUp:
			playIdle(*this);
			if (_male_carrier)
				playIdle(*_male_carrier);
			if (_milena_sister) {
				if (_sister_is_standing)
					playIdle(*_milena_sister);
				else
					_milena_sister->playAnimationReverseOnce(ANIM_DEATH);
			}
			break;
		case CarryState::Dispersing:
			if (_arrival_hub.radius <= 0.0f) {
				if (_engine)
					_arrival_hub = resolveHub(*_engine).value_or(HubDestination{_destination, _hub_radius_fallback});
				else
					_arrival_hub = HubDestination{_destination, _hub_radius_fallback};
			}
			startDispersal();
			break;
		case CarryState::Arrived:
			promoteMainEntityToMilenaSister(!saved_main_is_milena_sister);
			playIdle(*this);
			if (_male_carrier) {
				if (!state.contains("male_carrier")) {
					_male_carrier->setX(getX() + 0.9f);
					_male_carrier->setY(getY() + 0.35f);
					_male_carrier->setAltitude(getAltitude());
				}
				playIdle(*_male_carrier);
			}
			if (_milena_sister) {
				if (!state.contains("milena_sister")) {
					_milena_sister->setX(getX() - 0.9f);
					_milena_sister->setY(getY() + 0.45f);
					_milena_sister->setAltitude(getAltitude());
				}
				playIdle(*_milena_sister);
			}
			break;
		}
	}

} // namespace Nawia::Entity
