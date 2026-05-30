#include "StoryNpc.h"

#include <Engine.h>
#include <Logger.h>
#include <Map.h>
#include <MiniMushroomInfected.h>
#include <Quest.h>
#include <SoundIds.h>

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <json.hpp>

namespace Nawia::Entity {

	namespace {
		constexpr const char* MUSHROOM_MODEL = "assets/models/actors/gzib/mushroom_raylib_fixed.glb";
		constexpr const char* MUSHROOM_FALLBACK_MODEL = "assets/models/actors/cat/cat_bounce.glb";
		constexpr const char* VILLAGE_HEAD_MODEL = "assets/models/actors/village_head/village_head.glb";
		constexpr const char* BABA_YAGA_MODEL = "assets/models/actors/szeptucha/baba_yaga.glb";
		constexpr const char* WANDA_CORPSE_MODEL = "assets/models/actors/wanda/woman_dress.glb";
		constexpr float FOLLOW_STOP_DISTANCE = 0.45f;
		constexpr float MUSHROOM_TARGET_HEIGHT = 3.6f;
		constexpr float MUSHROOM_FALLBACK_SCALE = 15.0f;
		constexpr float MUSHROOM_MAX_SCALE = 50000.0f;
		constexpr float IDLE_LOOK_AT_PLAYER_INTERVAL = 0.25f;
		constexpr float VILLAGE_HEAD_SCALE = 1.55f;
		constexpr float VILLAGE_HEAD_STOP_DISTANCE = 0.65f;

		nlohmann::json loadJsonDocument(const std::string& path) {
			std::ifstream file(path);
			if (!file.is_open()) {
				Nawia::Core::Logger::errorLog("StoryNpc: nie mozna otworzyc JSON: " + path);
				return {};
			}

			nlohmann::json data;
			try {
				file >> data;
			} catch (const nlohmann::json::parse_error&) {
				Nawia::Core::Logger::errorLog("StoryNpc: blad parsowania JSON: " + path);
				return {};
			}
			return data;
		}

		const nlohmann::json& getNpcDialogueConfig() {
			static const nlohmann::json config = loadJsonDocument("assets/data/npc_dialogues.json");
			return config;
		}
	}

	StoryNpc::StoryNpc(const std::string& name, const float x, const float y)
		: InteractiveClickable(name, x, y, nullptr, 1)
	{
		_type = EntityType::NPCStatic;
		setFaction(Faction::None);
	}

	void StoryNpc::configureMushroom(Core::Engine* engine, const std::string& follow_checkpoint_name) {
		_engine = engine;
		_follow_checkpoint_name = follow_checkpoint_name;
		_can_follow = true;
		_type = EntityType::NPCActor;
		_home_position = getCenter();
		setMovementSpeed(2.4f);

		bool using_fallback_model = false;
		replaceModel(MUSHROOM_MODEL, false);
		if (!hasModelLoaded()) {
			Core::Logger::errorLog("StoryNpc: failed to load mushroom model " + std::string(MUSHROOM_MODEL));
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
			addAnimation("idle", MUSHROOM_MODEL, 0);
			addAnimation("stand_up", MUSHROOM_MODEL, 2);
			addAnimation("die", MUSHROOM_MODEL, 4);
			addAnimation("run", MUSHROOM_MODEL, 6);
			addAnimation("sneak", MUSHROOM_MODEL, 9);
			addAnimation("walk", MUSHROOM_MODEL, 11);
			addAnimation("talk", MUSHROOM_MODEL, 16);

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

	void StoryNpc::configureVillageHead(Core::Engine* engine) {
		_engine = engine;
		_is_village_head = true;
		_type = EntityType::NPCActor;
		setScale(VILLAGE_HEAD_SCALE);
		setMovementSpeed(2.0f);
		loadModel(VILLAGE_HEAD_MODEL);
		loadAnimationBundle(VILLAGE_HEAD_MODEL);
		playAnimation("Idle", true, false, 0, true);
		_dialogue_stage_key = "village_head";
		setDialogue(buildDialogueFromConfig("village_head"));
	}

	void StoryNpc::configureSzeptucha(Core::Engine* engine) {
		_engine = engine;
		_can_follow = false;
		_type = EntityType::NPCStatic;
		_dialogue_stage_key = "szeptucha";
		replaceModel(BABA_YAGA_MODEL, false);

		if (hasModelLoaded()) {
			const BoundingBox bounds = GetModelBoundingBox(getModel());
			const float model_height = bounds.max.y - bounds.min.y;
			float computed_scale = 2.0f;
			if (model_height > 1e-8f)
				computed_scale = std::clamp(3.2f / model_height, 0.1f, 30.0f);

			setScale(computed_scale);
			const float center_x = 0.5f * (bounds.min.x + bounds.max.x);
			const float center_z = 0.5f * (bounds.min.z + bounds.max.z);
			getModel().transform = MatrixMultiply(
				MatrixTranslate(-center_x, -bounds.min.y, -center_z),
				getModel().transform);
		}

		setPlaceholderDialogue("Szeptucha", "...");
	}

	void StoryNpc::configureWandaCorpse(Core::Engine* engine) {
		_engine = engine;
		_is_wanda_corpse = true;
		_type = EntityType::NPCStatic;
		_dialogue_stage_key = "wanda_corpse";
		setFaction(Faction::None);
		replaceModel(WANDA_CORPSE_MODEL, false);
		if (hasModelLoaded()) {
			const BoundingBox bounds = GetModelBoundingBox(getModel());
			const float center_x = 0.5f * (bounds.min.x + bounds.max.x);
			const float center_z = 0.5f * (bounds.min.z + bounds.max.z);
			getModel().transform = MatrixMultiply(
				MatrixMultiply(
					MatrixTranslate(-center_x, -bounds.min.y + 0.14f, -center_z),
					MatrixRotateX(PI / 2.0f)),
				getModel().transform);
		}
		setScale(1.65f);
		setDialogue(buildDialogueFromConfig("wanda_corpse"));
	}

	void StoryNpc::onInteract(Entity& instigator) {
		if (_is_wanda_corpse) {
			instigator.rotateTowardsCenter(getCenter().x, getCenter().y);
			return;
		}

		rotateTowardsCenter(instigator.getCenter().x, instigator.getCenter().y);
		instigator.rotateTowardsCenter(getCenter().x, getCenter().y);

		if (_can_follow)
			refreshMushroomDialogue();

		if (_is_village_head) {
			if (getAnimationFrameCount("Idle") > 0)
				playAnimation("Idle", true, false, 0, true);
			return;
		}

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

	bool StoryNpc::isMouseOver(const float screen_x, const float screen_y, const Camera3D& camera) const {
		if (_is_village_head && _survivor_quest_started)
			return false;

		if (_is_wanda_corpse && _wanda_corpse_inspected)
			return false;

		if (Entity::isMouseOver(screen_x, screen_y, camera))
			return true;

		if (!_is_wanda_corpse)
			return false;

		const Ray mouse_ray = GetScreenToWorldRay(Vector2{screen_x, screen_y}, camera);
		const Vector3 pos = getWorldPos3D();
		constexpr float click_half_width = 1.15f;
		constexpr float click_height = 0.9f;
		const BoundingBox click_box = {
			Vector3{pos.x - click_half_width, pos.y, pos.z - click_half_width},
			Vector3{pos.x + click_half_width, pos.y + click_height, pos.z + click_half_width}
		};

		return GetRayCollisionBox(mouse_ray, click_box).hit;
	}

	bool StoryNpc::canInteract() const {
		if (_is_wanda_corpse)
			return !_wanda_corpse_inspected;

		if (_is_village_head && _survivor_quest_started)
			return false;

		if (_can_follow)
			return hasMushroomDialogueAvailable();

		return true;
	}

	void StoryNpc::update(const float delta_time) {
		if (isDormant())
			return;

		Entity::update(delta_time);

		if (_playing_talk) {
			const bool dialogue_open = _engine && _engine->getUIHandler().isDialogueOpen();
			if (!dialogue_open) {
				_playing_talk = false;
				if (getAnimationFrameCount("Idle") > 0 || getAnimationFrameCount("idle") > 0)
					playAnimation(getAnimationFrameCount("Idle") > 0 ? "Idle" : "idle");
			}

			if (_can_follow) {
				rotateToPlayerOnInterval(delta_time);
				updateProceduralMushroomAnimation(delta_time);
				updateMovementSound(Audio::SoundPath::GzibWalk, false);
			}
			return;
		}

		if (_can_follow)
			updateMushroomFollow(delta_time);
		else if (_is_village_head)
			updateVillageHeadTravel(delta_time);

		if (_can_follow)
			updateMovementSound(Audio::SoundPath::GzibWalk, _is_moving, 0.42f, 1.12f);

		if (_use_procedural_mushroom_animation) {
			if (!_is_moving)
				rotateToPlayerOnInterval(delta_time);
			updateProceduralMushroomAnimation(delta_time);
		} else if (!_is_moving && getAnimationFrameCount("idle") > 0) {
			playAnimation("idle");
		}
	}

	float StoryNpc::getInteractionRange() {
		if (_is_wanda_corpse)
			return 3.2f * 3.2f;

		return 2.4f * 2.4f;
	}

	void StoryNpc::setPlaceholderDialogue(const std::string& speaker, const std::string& text) {
		Game::DialogueNode node;
		node.id = 0;
		node.speaker_name = speaker;
		node.text = text;

		Game::DialogueOption option;
		option.text = "Rozumiem.";
		option.next_node_id = -1;
		node.options.push_back(option);

		Game::DialogueTree tree;
		tree.addNode(node);
		setDialogue(tree);
	}

	void StoryNpc::onDialogueClosed(const int node_id, const bool completed) {
		_last_completed_dialogue_stage = completed ? _dialogue_stage_key : "";
		_dialogue_resume_node = completed ? 0 : node_id;
	}

	bool StoryNpc::shouldNotifyQuestTalkOnDialogueComplete() const {
		if (getName() != "Gzib")
			return true;

		return _dialogue_stage_key == "intro" ||
			_dialogue_stage_key == "follow" ||
			_dialogue_stage_key == "report";
	}

	void StoryNpc::handleQuestTalkCompleted(Core::Engine& engine) {
		if (_is_village_head && !_survivor_quest_started) {
			engine.getQuestManager().update(&engine);
			_survivor_quest_started = true;
			if (engine.getQuestManager().startQuest("find_survivors"))
				engine.getUIHandler().showNotification("Nowy quest: Znajdz ocalencow", 4.0f);
			startVillageHeadRouteToPlayerSpawn();
			return;
		}

		if (_is_wanda_corpse) {
			if (_wanda_corpse_inspected)
				return;
			_wanda_corpse_inspected = true;
			const Vector2 corpse_center = getCenter();
			engine.queueWczoraCorpseInspected(corpse_center);
			return;
		}

		if (getName() != "Gzib")
			return;

		if (_last_completed_dialogue_stage == "report" && !_return_started) {
			startMushroomRoute(TravelMode::ReturningHome);
			return;
		}

		if (_last_completed_dialogue_stage == "follow") {
			const int rescued_count = getRescuedMushroomCount();
			if (rescued_count <= 0)
				return;

			for (int i = 0; i < rescued_count; ++i)
				engine.getQuestManager().notifyKill("Robal");

			engine.getQuestManager().update(&engine);
		}
	}

	void StoryNpc::refreshMushroomDialogue() {
		if (!_engine)
			return;

		const auto* intro_quest = _engine->getQuestManager().getQuest("gzib_talk_intro");
		const auto* follow_quest = _engine->getQuestManager().getQuest("gzib_follow");
		const auto* report_quest = _engine->getQuestManager().getQuest("gzib_report_brothers");
		std::string stage_key = "idle";

		if (_dialogue_resume_node > 0 && _dialogue_stage_key == "report") {
			setDialogue(buildDialogueFromConfig("gzib_report"));
			return;
		}

		if (_dialogue_resume_node > 0 && _dialogue_stage_key == "follow") {
			setDialogue(buildDialogueFromConfig("gzib_follow"));
			return;
		}

		if (_dialogue_resume_node > 0 && _dialogue_stage_key == "intro") {
			setDialogue(buildDialogueFromConfig("gzib_intro"));
			return;
		}

		if (report_quest && report_quest->isActive()) {
			stage_key = "report";
			if (_dialogue_stage_key != stage_key) {
				_dialogue_stage_key = stage_key;
				_dialogue_resume_node = 0;
			}
			setDialogue(buildDialogueFromConfig("gzib_report"));
			return;
		}

		const bool follow_checkpoint_ready = follow_quest && follow_quest->isActive() &&
			(_reached_follow_checkpoint ||
			 (!follow_quest->objectives.empty() && follow_quest->objectives.front().isCompleted()));
		if (follow_checkpoint_ready) {
			stage_key = "follow";
			if (_dialogue_stage_key != stage_key) {
				_dialogue_stage_key = stage_key;
				_dialogue_resume_node = 0;
			}
			if (areAllMushroomsAlreadyRescued()) {
				setDialogue(buildDialogueFromConfig("gzib_follow_done"));
			} else {
				setDialogue(buildDialogueFromConfig("gzib_follow"));
			}
			return;
		}

		if (!intro_quest || intro_quest->isActive() || intro_quest->isAvailable()) {
			stage_key = "intro";
			if (_dialogue_stage_key != stage_key) {
				_dialogue_stage_key = stage_key;
				_dialogue_resume_node = 0;
			}
			setDialogue(buildDialogueFromConfig("gzib_intro"));
			return;
		}

		if (_dialogue_stage_key != stage_key) {
			_dialogue_stage_key = stage_key;
			_dialogue_resume_node = 0;
		}
		setPlaceholderDialogue("Gzib", "GZIBY CZEKAJA.");
	}

	Game::DialogueTree StoryNpc::buildLinearDialogue(
		const std::vector<std::pair<std::string, std::string>>& lines,
		const std::string& final_option_text) const {
		Game::DialogueTree tree;
		int node_id = 0;
		for (size_t i = 0; i < lines.size();) {
			Game::DialogueNode node;
			node.id = node_id;
			node.speaker_name = lines[i].first;
			node.text = lines[i].second;

			Game::DialogueOption option;
			size_t next_line = i + 1;
			if (next_line < lines.size() && lines[next_line].first == "Jarko") {
				option.text = lines[next_line].second;
				next_line++;
			} else {
				option.text = (next_line < lines.size()) ? "..." : final_option_text;
			}

			option.next_node_id = (next_line < lines.size()) ? node_id + 1 : -1;
			node.options.push_back(option);
			tree.addNode(node);

			i = next_line;
			node_id++;
		}

		return tree;
	}

	Game::DialogueTree StoryNpc::buildVoicedLinearDialogue(
		const std::vector<DialogueLine>& lines,
		const std::string& final_option_text) const {
		Game::DialogueTree tree;
		for (size_t i = 0; i < lines.size(); ++i) {
			Game::DialogueNode node;
			node.id = static_cast<int>(i);
			node.speaker_name = lines[i].speaker;
			node.text = lines[i].text;
			node.voice_path = lines[i].voice_path;

			Game::DialogueOption option;
			option.text = (i + 1 < lines.size()) ? "..." : final_option_text;
			option.next_node_id = (i + 1 < lines.size()) ? static_cast<int>(i + 1) : -1;
			node.options.push_back(option);
			tree.addNode(node);
		}

		return tree;
	}

	Game::DialogueTree StoryNpc::buildDialogueFromConfig(const std::string& key) const {
		const auto& config = getNpcDialogueConfig();
		if (!config.contains(key) || !config[key].is_object())
			return {};

		const auto& dialogue = config[key];
		const std::string final_option = dialogue.value("final_option", "...");
		const auto& lines_json = dialogue["lines"];
		if (!lines_json.is_array())
			return {};

		bool has_voice = false;
		std::vector<DialogueLine> voiced_lines;
		std::vector<std::pair<std::string, std::string>> plain_lines;
		for (const auto& line_json : lines_json) {
			DialogueLine line;
			line.speaker = line_json.value("speaker", "");
			line.text = line_json.value("text", "");
			line.voice_path = line_json.value("voice_path", "");
			has_voice = has_voice || !line.voice_path.empty();
			plain_lines.emplace_back(line.speaker, line.text);
			voiced_lines.push_back(std::move(line));
		}

		return has_voice
			? buildVoicedLinearDialogue(voiced_lines, final_option)
			: buildLinearDialogue(plain_lines, final_option);
	}

	void StoryNpc::updateMushroomFollow(const float delta_time) {
		if (!_engine)
			return;

		if (_travel_mode == TravelMode::None) {
			if (_reached_follow_checkpoint)
				return;

			const auto* follow_quest = _engine->getQuestManager().getQuest("gzib_follow");
			if (!follow_quest || !follow_quest->isActive())
				return;

			startMushroomRoute(TravelMode::ToBrothers);
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
			advanceMushroomRoute();
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

	void StoryNpc::updateVillageHeadTravel(const float delta_time) {
		if (!_village_head_walking_to_spawn)
			return;

		const float distance = Vector2Distance(getCenter(), _village_head_destination);
		if (distance <= VILLAGE_HEAD_STOP_DISTANCE) {
			stopPathMovement();
			_village_head_walking_to_spawn = false;
			if (getAnimationFrameCount("Idle") > 0)
				playAnimation("Idle", true, false, 0, true);
			return;
		}

		if (!_follow_path_requested) {
			buildPathToPoint(_village_head_destination);
			_follow_path_requested = true;
		}

		updatePathMovement(delta_time);

		if (_is_moving) {
			if (getAnimationFrameCount("Walk") > 0)
				playAnimation("Walk");
			else if (getAnimationFrameCount("walk") > 0)
				playAnimation("walk");
		}
	}

	void StoryNpc::startVillageHeadRouteToPlayerSpawn() {
		if (!_engine)
			return;

		const auto player = _engine->getPlayer();
		if (!player)
			return;

		Vector3 destination = {player->getRespawnPoint().x, getAltitude(), player->getRespawnPoint().y};
		if (_engine->getCurrentMap() && _engine->getCurrentMap()->getNavMesh().isReady())
			destination = _engine->getCurrentMap()->getNavMesh().getClosestWalkablePosition({destination.x, 0.0f, destination.z});

		_village_head_destination = {destination.x, destination.z};
		_village_head_walking_to_spawn = true;
		_follow_path_requested = false;
		buildPathToPoint(_village_head_destination);
	}

	void StoryNpc::startMushroomRoute(const TravelMode mode) {
		_travel_waypoints = collectOrderedFollowWaypoints(mode == TravelMode::ReturningHome);
		if (mode == TravelMode::ReturningHome)
			_travel_waypoints.push_back(_home_position);

		_travel_waypoint_index = 0;
		_travel_mode = _travel_waypoints.empty() ? TravelMode::None : mode;
		_follow_path_requested = false;
		_return_started = mode == TravelMode::ReturningHome;

		if (_travel_mode != TravelMode::None) {
			if (mode == TravelMode::ReturningHome)
				sendPurifiedMushroomsHome();
			buildPathToPoint(_travel_waypoints.front());
		}
	}

	void StoryNpc::advanceMushroomRoute() {
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

	void StoryNpc::buildPathToPoint(const Vector2 target) {
		_current_path.clear();

		if (_engine && _engine->getCurrentMap() && _engine->getCurrentMap()->getNavMesh().isReady()) {
			_current_path = _engine->getCurrentMap()->findPath(getWorldPos3D(), {target.x, getAltitude(), target.y});
			Core::Logger::debugLog("StoryNpc: sciezka Gziba ma " + std::to_string(_current_path.size()) + " punktow");
		}

		if (_current_path.empty()) {
			_current_path.push_back(target);
		}

		trimCurrentPathStart();

		if (!_current_path.empty())
			moveTo(_current_path.front().x, _current_path.front().y);
		else
			stopPathMovement();
	}

	void StoryNpc::trimCurrentPathStart() {
		if (_current_path.empty())
			return;

		const Vector2 first_path_point = _current_path.front();
		const float dx = first_path_point.x - getCenter().x;
		const float dy = first_path_point.y - getCenter().y;
		if (dx * dx + dy * dy < 0.1f)
			_current_path.erase(_current_path.begin());
	}

	void StoryNpc::updatePathMovement(const float delta_time) {
		if (!_is_moving && !_current_path.empty()) {
			_current_path.erase(_current_path.begin());

			if (!_current_path.empty())
				moveTo(_current_path.front().x, _current_path.front().y);
		}

		updateMovement(delta_time);
	}

	void StoryNpc::stopPathMovement() {
		_current_path.clear();
		_is_moving = false;
		setVelocity(0.0f, 0.0f);
	}

	void StoryNpc::updateProceduralMushroomAnimation(const float delta_time) {
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

	void StoryNpc::rotateToPlayerOnInterval(const float delta_time) {
		_look_at_player_timer -= delta_time;
		if (_look_at_player_timer > 0.0f)
			return;

		_look_at_player_timer = IDLE_LOOK_AT_PLAYER_INTERVAL;
		if (const auto player = _engine ? _engine->getPlayer() : nullptr)
			rotateTowardsCenter(player->getCenter().x, player->getCenter().y);
	}

	std::vector<Vector2> StoryNpc::collectOrderedFollowWaypoints(const bool reverse_to_home) const {
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
			// For return, build from home backwards, then reverse to go from current place to home.
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

	void StoryNpc::sendPurifiedMushroomsHome() {
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

	int StoryNpc::getRescuedMushroomCount() const {
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

	int StoryNpc::getRequiredRescueCount() const {
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

	bool StoryNpc::areAllMushroomsAlreadyRescued() const {
		const int rescued_count = getRescuedMushroomCount();
		return rescued_count > 0 && rescued_count >= getRequiredRescueCount();
	}

	bool StoryNpc::hasMushroomDialogueAvailable() const {
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
