#include "GenericStoryNpc.h"

#include <Engine.h>
#include <EntityManager.h>
#include <Level.h>
#include <Logger.h>
#include <Map.h>
#include <Player.h>
#include <QuestManager.h>
#include <UIHandler.h>

#include <raymath.h>

#include <cmath>
#include <functional>
#include <utility>

namespace Nawia::Entity {

	namespace {
		constexpr const char* QUEST_RETURN_TO_HERBALIST_FINAL = "return_to_herbalist_final";
		constexpr const char* QUEST_CLEAR_SPIDER_NEST = "clear_spider_nest";
		constexpr const char* QUEST_RETURN_AFTER_SPIDER = "return_to_herbalist_after_spider";
		constexpr const char* QUEST_TALK_TO_MILENA_SISTER = "talk_to_milena_sister";
		constexpr const char* SPIDER_NAME = "Straszny pajak";
		constexpr float BABA_YAGA_IDLE_BOB_AMPLITUDE = 0.08f;
		constexpr float BABA_YAGA_IDLE_BOB_SPEED = 1.25f;
		constexpr float BABA_YAGA_TILT_DEGREES = 2.8f;
		constexpr float BABA_YAGA_TILT_SPEED = 0.9f;

		std::string readStringAlias(const nlohmann::json& data, const std::initializer_list<const char*> keys) {
			for (const char* key : keys) {
				if (data.contains(key) && data[key].is_string())
					return data[key].get<std::string>();
			}
			return "";
		}

		bool questCompleted(Core::Engine* engine, const std::string& quest_id) {
			if (!engine)
				return false;

			const auto* quest = engine->getQuestManager().getQuest(quest_id);
			return quest && quest->isCompleted();
		}

		bool questActive(Core::Engine* engine, const std::string& quest_id) {
			if (!engine)
				return false;

			const auto* quest = engine->getQuestManager().getQuest(quest_id);
			return quest && quest->isActive();
		}

		bool questActiveOrCompleted(Core::Engine* engine, const std::string& quest_id) {
			return questActive(engine, quest_id) || questCompleted(engine, quest_id);
		}

		void startQuestIfPossible(Core::Engine& engine, const std::string& quest_id) {
			engine.getQuestManager().update(&engine);
			engine.getQuestManager().startQuest(quest_id);
			engine.getQuestManager().update(&engine);
		}

		void completeQuestIfActive(Core::Engine& engine, const std::string& quest_id) {
			if (questActive(&engine, quest_id)) {
				engine.getQuestManager().completeQuest(quest_id, &engine);
				engine.getQuestManager().update(&engine);
			}
		}

		Game::DialogueNode makeDialogueNode(
			const int id,
			std::string speaker,
			std::string text,
			std::vector<Game::DialogueOption> options,
			std::string voice_path = "")
		{
			Game::DialogueNode node;
			node.id = id;
			node.speaker_name = std::move(speaker);
			node.text = std::move(text);
			node.voice_path = std::move(voice_path);
			node.options = std::move(options);
			return node;
		}

		Game::DialogueOption makeDialogueOption(std::string text, const int next_node_id, std::function<void()> action = nullptr) {
			Game::DialogueOption option;
			option.text = std::move(text);
			option.next_node_id = next_node_id;
			option.action = std::move(action);
			return option;
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
		_use_baba_yaga_idle_visual = data.value("idle_visual_effect", "") == "baba_yaga";
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

		if (_use_baba_yaga_idle_visual)
			replaceModel(_model_path, data.value("rotate_model", false));
		else
			loadModel(_model_path, data.value("rotate_model", false));

		if (_model_path.find("female_warrior") != std::string::npos)
			hideMeshIndex(1); // Mesh 1 to miecz w female_warrior.glb.
		if (!_use_baba_yaga_idle_visual && !_animation_bundle_path.empty())
			loadAnimationBundle(_animation_bundle_path);

		if (data.value("use_indexed_animation_aliases", false)) {
			addAnimation("death", _model_path, data.value("death_animation_index", 0));
			addAnimation("idle", _model_path, data.value("idle_animation_index", 4));
			addAnimation("walk", _model_path, data.value("walk_animation_index", 22));
			addAnimation("walk_back", _model_path, data.value("walk_back_animation_index", 17));
			if (data.contains("cast_animation_index") || _model_path.find("witch") != std::string::npos)
				addAnimation("cast", _model_path, data.value("cast_animation_index", 7));
		}

		const float target_height = data.value("target_height", 0.0f);
		if (target_height > 0.0f)
			fitLoadedModelToHeight(target_height);
		else
			setScale(data.value("scale", getScale()));

		if (_use_baba_yaga_idle_visual && hasModelLoaded()) {
			_idle_visual_base_transform = getModel().transform;
			_has_idle_visual_base_transform = true;
		}
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
		if (isHerbalist())
			refreshHerbalistDialogue();

		rotateTowardsCenter(instigator.getCenter().x, instigator.getCenter().y);
		instigator.rotateTowardsCenter(getCenter().x, getCenter().y);
		playIdleAnimation();
	}

	bool GenericStoryNpc::canInteract() const {
		if (!_can_talk)
			return false;

		if (isHerbalist())
			return canHerbalistInteract();

		if (_disable_interaction_after_talk && _talk_completed)
			return false;

		return !_walking_to_destination && !_arrived;
	}

	void GenericStoryNpc::render(const Camera3D& camera) {
		// Po zakonczeniu trasy lub interakcji NPC nie powinien podswietlac sie na hover.
		if (!canInteract())
			setHovered(false);

		Entity::render(camera);
	}

	void GenericStoryNpc::update(const float delta_time) {
		if (isDormant())
			return;

		updateBabaYagaIdleVisual(delta_time);

		if (_walking_to_destination) {
			Entity::update(delta_time);
			if (!isAnimationLocked())
				updateRouteToDestination(delta_time);
			return;
		}

		StoryNpc::update(delta_time);
	}

	void GenericStoryNpc::updateBabaYagaIdleVisual(const float delta_time) {
		if (!_use_baba_yaga_idle_visual)
			return;

		_idle_visual_time += delta_time;
		if (_has_idle_visual_base_transform && hasModelLoaded()) {
			const float tilt = std::sin(_idle_visual_time * BABA_YAGA_TILT_SPEED) * BABA_YAGA_TILT_DEGREES * DEG2RAD;
			getModel().transform = MatrixMultiply(_idle_visual_base_transform, MatrixRotateX(tilt));
		}
	}

	Vector3 GenericStoryNpc::getWorldPos3D() const {
		const float bob = _use_baba_yaga_idle_visual
			? std::sin(_idle_visual_time * BABA_YAGA_IDLE_BOB_SPEED) * BABA_YAGA_IDLE_BOB_AMPLITUDE
			: 0.0f;
		return {_pos.x, _altitude + bob, _pos.y};
	}

	void GenericStoryNpc::handleQuestTalkCompleted(Core::Engine& engine) {
		if (isHerbalist())
			return;

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
		if (!_complete_quest_id.empty())
			engine.getQuestManager().update(&engine);

		if (!_fail_quest_id.empty())
			engine.getQuestManager().failQuest(_fail_quest_id, &engine);
	}

	bool GenericStoryNpc::isHerbalist() const {
		return _npc_class_name == "herbalist";
	}

	bool GenericStoryNpc::canHerbalistInteract() const {
		if (!_engine)
			return false;

		if (questActiveOrCompleted(_engine, QUEST_RETURN_AFTER_SPIDER))
			return true;

		if (questCompleted(_engine, QUEST_CLEAR_SPIDER_NEST))
			return true;

		if (questActive(_engine, QUEST_CLEAR_SPIDER_NEST))
			return isSpiderNestCleared();

		return questActive(_engine, QUEST_RETURN_TO_HERBALIST_FINAL);
	}

	bool GenericStoryNpc::isMilenaSisterAlive() const {
		return questCompleted(_engine, "rescue_forest_survivors");
	}

	bool GenericStoryNpc::isMilenaSisterOptionalTalkCompleted() const {
		return !isMilenaSisterAlive() || questCompleted(_engine, QUEST_TALK_TO_MILENA_SISTER);
	}

	bool GenericStoryNpc::isSpiderNestCleared() const {
		if (questCompleted(_engine, QUEST_CLEAR_SPIDER_NEST))
			return true;

		if (!_engine)
			return false;

		auto* level = _engine->getLevelManager().getCurrentLevel();
		if (!level)
			return false;

		for (const auto& spawn_point : level->getSpawnManager().getSpawnPoints()) {
			if (spawn_point.entity_type != "spider")
				continue;
			if (spawn_point.entity_data.value("name", "") != SPIDER_NAME)
				continue;
			return spawn_point.entity && spawn_point.entity->isDead();
		}

		return false;
	}

	void GenericStoryNpc::refreshHerbalistDialogue() {
		setDialogueStageKey("herbalist_dynamic");
		setDialogue(buildHerbalistDialogue());
	}

	void GenericStoryNpc::startHerbalistSpiderQuest(Core::Engine& engine) const {
		completeQuestIfActive(engine, QUEST_RETURN_TO_HERBALIST_FINAL);
		startQuestIfPossible(engine, QUEST_CLEAR_SPIDER_NEST);

		if (isSpiderNestCleared()) {
			completeQuestIfActive(engine, QUEST_CLEAR_SPIDER_NEST);
			engine.getQuestManager().update(&engine);
		}
	}

	void GenericStoryNpc::startMilenaSisterOptionalQuest(Core::Engine& engine) const {
		startQuestIfPossible(engine, QUEST_TALK_TO_MILENA_SISTER);
		engine.getUIHandler().showNotification("Opcjonalnie: porozmawiaj z siostra Mileny", 4.0f);
	}

	void GenericStoryNpc::finishWczoraLevel(Core::Engine& engine) const {
		completeQuestIfActive(engine, QUEST_RETURN_AFTER_SPIDER);
		engine.getQuestManager().notifyCheckpointReached("wczora_epilogue_complete");
		engine.getQuestManager().update(&engine);
		engine.notifyStoryEvent("wczora_outro_requested", getCenter());
	}

	Game::DialogueTree GenericStoryNpc::buildHerbalistDialogue() const {
		Game::DialogueTree tree;
		if (!_engine)
			return tree;

		const bool sister_alive = isMilenaSisterAlive();
		const bool sister_talk_done = isMilenaSisterOptionalTalkCompleted();
		const bool spider_cleared = isSpiderNestCleared();

		if (spider_cleared) {
			if (sister_alive && !sister_talk_done) {
				tree.addNode(makeDialogueNode(0, "Zielarz",
					"Wiec wielki katnik juz nie pilnuje rumowiska. Dobrze. Mozemy zabrac ludzi i narzedzia, ale zanim ruszymy... jej siostra czeka. Chcesz z nia porozmawiac, zanim wezmiemy sie do odgruzowania?",
					{
						makeDialogueOption("Tak. Najpierw z nia porozmawiam.", -1, [engine = _engine, this]() {
							startHerbalistSpiderQuest(*engine);
							startMilenaSisterOptionalQuest(*engine);
						}),
						makeDialogueOption("Nie. Konczmy to.", 1, [engine = _engine, this]() {
							startHerbalistSpiderQuest(*engine);
						})
					},
					"assets/audio/dialogues/StoryFinal/herbalist_dynamic_spider_done_00.wav"));
			} else {
				tree.addNode(makeDialogueNode(0, "Zielarz",
					"Katnik nie zyje, a przejscie czeka. Ludzie sa gotowi. Jesli odprawimy dziady przy potoku nad Twierdza Kamienna, moze jeszcze uda sie odwrocic gniew, ktory spadl na Wczore.",
					{makeDialogueOption("Ruszajmy.", 1)},
					"assets/audio/dialogues/StoryFinal/herbalist_dynamic_spider_done_00.wav"));
			}

			tree.addNode(makeDialogueNode(1, "Logos",
				"Logos dowiedzial sie, co stalo sie z jego ludem. Nie po to, by zlozyc wine na martwych albo na bogow, ale zeby sprobowac naprawic zerwana wiez. Najpierw dziady. Potem droga za Milena.",
				{makeDialogueOption("Dalej.", 2)},
				"assets/audio/dialogues/StoryFinal/herbalist_dynamic_epilogue_logos_01.wav"));
			tree.addNode(makeDialogueNode(2, "Zielarz",
				"Potok nad Twierdza Kamienna bedzie dobrym miejscem. Woda poniesie slowa do tych, ktorzy nie maja juz ust, a kamien przypomni zywym, ze nie sa sami na tej ziemi.",
				{makeDialogueOption("Do Twierdzy Kamiennej.", -1, [engine = _engine, this]() {
					finishWczoraLevel(*engine);
				})},
				"assets/audio/dialogues/StoryFinal/herbalist_dynamic_epilogue_02.wav"));
			return tree;
		}

		tree.addNode(makeDialogueNode(0, "Logos",
			"Wiedzma mowila o dziadach. O winie, bogach i zmarlych, ktorzy nie dostali naleznego glosu. Ale ja musze ruszyc za Milena. Wiesz, dokad poszla?",
			{makeDialogueOption("Dalej.", 1)},
			"assets/audio/dialogues/StoryFinal/herbalist_dynamic_intro_logos_00.wav"));

		if (sister_alive) {
			tree.addNode(makeDialogueNode(1, "Zielarz",
				"Mam dobre i zle wiesci. Dobre sa takie, ze ranna, ktora ocaliles, to siostra Mileny. Jeszcze slaba, ale przytomna. Powie ci wiecej niz ja.",
				{makeDialogueOption("Zyje... Bogowie. A zle wiesci?", 2)},
				"assets/audio/dialogues/StoryFinal/herbalist_dynamic_sister_alive_01.wav"));
		} else {
			tree.addNode(makeDialogueNode(1, "Zielarz",
				"Mam zle wiesci, Logosie. Wiem tylko, ze Milena uciekla z wioski i ruszyla z innymi na zachod. Jej siostra nie dotarla do mnie zywa.",
				{makeDialogueOption("Kurwa...", 2)},
				"assets/audio/dialogues/StoryFinal/herbalist_dynamic_sister_dead_01.wav"));
		}

		tree.addNode(makeDialogueNode(2, "Zielarz",
			"Przejscie jest zawalone. Kamien siedzi na kamieniu jak przeklenstwo, a w rumowisku gniazdo urzadzil wielki katnik. Dopoki to bydle tam zyje, nikt nie podejdzie z lopata ani modlitwa.",
			{makeDialogueOption("Czyli najpierw pajak.", 3)},
			"assets/audio/dialogues/StoryFinal/herbalist_dynamic_rubble_02.wav"));
		tree.addNode(makeDialogueNode(3, "Logos",
			"Zabije go. Potem odgruzujemy przejscie i odprawimy dziady tam, gdzie trzeba.",
			{makeDialogueOption(
				sister_alive ? "Porozmawiam z jej siostra i zajme sie katnikiem." : "Zajme sie katnikiem.",
				-1,
				[engine = _engine, this, sister_alive]() {
					startHerbalistSpiderQuest(*engine);
					if (sister_alive)
						startMilenaSisterOptionalQuest(*engine);
				})},
			"assets/audio/dialogues/StoryFinal/herbalist_dynamic_spider_quest_logos_03.wav"));

		return tree;
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
