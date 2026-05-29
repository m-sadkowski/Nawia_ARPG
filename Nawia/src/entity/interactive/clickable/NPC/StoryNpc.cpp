#include "StoryNpc.h"

#include <Engine.h>
#include <Logger.h>
#include <Map.h>
#include <MiniMushroomInfected.h>
#include <Quest.h>

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

	namespace {
		constexpr const char* MUSHROOM_MODEL = "assets/models/mushroom_raylib_fixed.glb";
		constexpr const char* MUSHROOM_FALLBACK_MODEL = "assets/models/cat_bounce.glb";
		constexpr const char* VILLAGE_HEAD_MODEL = "assets/models/village_head.glb";
		constexpr float FOLLOW_STOP_DISTANCE = 0.45f;
		constexpr float MUSHROOM_TARGET_HEIGHT = 3.6f;
		constexpr float MUSHROOM_FALLBACK_SCALE = 15.0f;
		constexpr float MUSHROOM_MAX_SCALE = 50000.0f;
		constexpr float IDLE_LOOK_AT_PLAYER_INTERVAL = 0.25f;
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
		setScale(1.0f);
		loadModel(VILLAGE_HEAD_MODEL);
		loadAnimationBundle(VILLAGE_HEAD_MODEL);
		playAnimation("Idle", true, false, 0, true);
		setPlaceholderDialogue("Soltys", "Jeszcze ustawimy tu dialog Soltysa po walce z Ropuchem.");
	}

	void StoryNpc::onInteract(Entity& /*instigator*/) {
		if (_can_follow)
			refreshMushroomDialogue();

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

			if (_use_procedural_mushroom_animation) {
				rotateToPlayerOnInterval(delta_time);
				updateProceduralMushroomAnimation(delta_time);
			}
			return;
		}

		if (_can_follow)
			updateMushroomFollow(delta_time);

		if (_use_procedural_mushroom_animation) {
			if (!_is_moving)
				rotateToPlayerOnInterval(delta_time);
			updateProceduralMushroomAnimation(delta_time);
		} else if (!_is_moving && getAnimationFrameCount("idle") > 0) {
			playAnimation("idle");
		}
	}

	float StoryNpc::getInteractionRange() {
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
		if (getName() != "Gzib" || _last_completed_dialogue_stage != "follow")
			return;

		const int rescued_count = getRescuedMushroomCount();
		if (rescued_count <= 0)
			return;

		for (int i = 0; i < rescued_count; ++i)
			engine.getQuestManager().notifyKill("Robal");

		engine.getQuestManager().update(&engine);
	}

	void StoryNpc::refreshMushroomDialogue() {
		if (!_engine)
			return;

		const auto* intro_quest = _engine->getQuestManager().getQuest("gzib_talk_intro");
		const auto* follow_quest = _engine->getQuestManager().getQuest("gzib_follow");
		const auto* report_quest = _engine->getQuestManager().getQuest("gzib_report_brothers");
		std::string stage_key = "idle";

		if (_dialogue_resume_node > 0 && _dialogue_stage_key == "report") {
			setDialogue(buildLinearDialogue({
				{"Gzib", "GZIBY URATOWANE! DZIEKUJE"},
				{"Gracz", "Wydaje mi sie ze ten ropuch tez bral udzial w ataku na wioske. Pamietam jak przez mgle jakis rechot."},
				{"Gzib", "GZIBY MOWIA, ZE W SADZAWCE SLYCHAC RE RE KUM KUM"},
				{"Gracz", "Pora go odwiedzic. Widziales moze innych mieszkancow wioski ktorzy tedy uciekali?"},
				{"Gzib", "NIE WIDZIAL. ALE NAGRODA OD GZIBOW - WEZ"}
			}, "Wez."));
			return;
		}

		if (_dialogue_resume_node > 0 && _dialogue_stage_key == "follow") {
			setDialogue(buildLinearDialogue({
				{"Gracz", "One wygladaja jakby juz nie chcialy sie z toba kolegowac."},
				{"Gzib", "WYTNIJ ROBAKI RATUJ GZIBY, RATUJ GZIBYYY"},
				{"Gracz", "No dobra, sprobuje"}
			}, "Sprobuje."));
			return;
		}

		if (_dialogue_resume_node > 0 && _dialogue_stage_key == "intro") {
			setDialogue(buildLinearDialogue({
				{"Gzib", "GZIBYYY, GDZIE MOJE GZIBYYY"},
				{"Gracz", "O, nie tylko ja mam omamy. Co sie stalo muchomorku?"},
				{"Gzib", "MOJE GZIBY, NIE MA GZIBOW"},
				{"Gracz", "Pomoc ci jakos?"},
				{"Gzib", "GZIBY, URATUJ, ZATRUTANE, ZACZAROWANOWANE"},
				{"Gracz", "Kto zaczarowal twoich kolegow?"},
				{"Gzib", "GRUBY ROPUCH. ROPUCH KRZYCZY ZE GO BRZUCH BOLI OD GZIBY"},
				{"Gracz", "No chyba ty jestes jakis rozjebany muchmorze."},
				{"Gzib", "CHODZ RATUJ GZIBY"}
			}, "Chodzmy."));
			return;
		}

		if (report_quest && report_quest->isActive()) {
			stage_key = "report";
			if (_dialogue_stage_key != stage_key) {
				_dialogue_stage_key = stage_key;
				_dialogue_resume_node = 0;
			}
			setDialogue(buildLinearDialogue({
				{"Gzib", "GZIBY URATOWANE! DZIEKUJE"},
				{"Gracz", "Wydaje mi sie ze ten ropuch tez bral udzial w ataku na wioske. Pamietam jak przez mgle jakis rechot."},
				{"Gzib", "GZIBY MOWIA, ZE W SADZAWCE SLYCHAC RE RE KUM KUM"},
				{"Gracz", "Pora go odwiedzic. Widziales moze innych mieszkancow wioski ktorzy tedy uciekali?"},
				{"Gzib", "NIE WIDZIAL. ALE NAGRODA OD GZIBOW - WEZ"}
			}, "Wez."));
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
				setDialogue(buildLinearDialogue({
					{"Gracz", "Robale juz wyciete. Gziby sa bezpieczne."},
					{"Gzib", "GZIBY URATOWANE! DZIEKUJE"}
				}, "Dobra."));
			} else {
				setDialogue(buildLinearDialogue({
					{"Gracz", "One wygladaja jakby juz nie chcialy sie z toba kolegowac."},
					{"Gzib", "WYTNIJ ROBAKI RATUJ GZIBY, RATUJ GZIBYYY"},
					{"Gracz", "No dobra, sprobuje"}
				}, "Sprobuje."));
			}
			return;
		}

		if (!intro_quest || intro_quest->isActive() || intro_quest->isAvailable()) {
			stage_key = "intro";
			if (_dialogue_stage_key != stage_key) {
				_dialogue_stage_key = stage_key;
				_dialogue_resume_node = 0;
			}
			setDialogue(buildLinearDialogue({
				{"Gzib", "GZIBYYY, GDZIE MOJE GZIBYYY"},
				{"Gracz", "O, nie tylko ja mam omamy. Co sie stalo muchomorku?"},
				{"Gzib", "MOJE GZIBY, NIE MA GZIBOW"},
				{"Gracz", "Pomoc ci jakos?"},
				{"Gzib", "GZIBY, URATUJ, ZATRUTANE, ZACZAROWANOWANE"},
				{"Gracz", "Kto zaczarowal twoich kolegow?"},
				{"Gzib", "GRUBY ROPUCH. ROPUCH KRZYCZY ZE GO BRZUCH BOLI OD GZIBY"},
				{"Gracz", "No chyba ty jestes jakis rozjebany muchmorze."},
				{"Gzib", "CHODZ RATUJ GZIBY"}
			}, "Chodzmy."));
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
			if (next_line < lines.size() && lines[next_line].first == "Gracz") {
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

	void StoryNpc::updateMushroomFollow(const float delta_time) {
		if (!_engine || _reached_follow_checkpoint)
			return;

		const auto* follow_quest = _engine->getQuestManager().getQuest("gzib_follow");
		if (!follow_quest || !follow_quest->isActive())
			return;

		const Entity* checkpoint = findEntityByName(_follow_checkpoint_name);
		if (!checkpoint)
			return;

		const Vector2 target = checkpoint->getCenter();
		const float distance = Vector2Distance(getCenter(), target);
		if (distance <= FOLLOW_STOP_DISTANCE) {
			_reached_follow_checkpoint = true;
			stopPathMovement();
			_engine->getQuestManager().notifyCheckpointReached(_follow_checkpoint_name);
			if (!_use_procedural_mushroom_animation && getAnimationFrameCount("idle") > 0)
				playAnimation("idle");
			return;
		}

		if (!_follow_path_requested) {
			buildPathToFollowCheckpoint(*checkpoint);
			_follow_path_requested = true;
		}

		updatePathMovement(delta_time);

		if (_is_moving && !_use_procedural_mushroom_animation && getAnimationFrameCount("walk") > 0)
			playAnimation("walk");
	}

	void StoryNpc::buildPathToFollowCheckpoint(const Entity& checkpoint) {
		_current_path.clear();

		if (_engine && _engine->getCurrentMap() && _engine->getCurrentMap()->getNavMesh().isReady()) {
			_current_path = _engine->getCurrentMap()->findPath(getWorldPos3D(), checkpoint.getWorldPos3D());
			Core::Logger::debugLog("StoryNpc: sciezka Gziba ma " + std::to_string(_current_path.size()) + " punktow");
		}

		if (_current_path.empty()) {
			const Vector2 target = checkpoint.getCenter();
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

	Entity* StoryNpc::findEntityByName(const std::string& name) const {
		if (!_engine)
			return nullptr;

		for (const auto& entity : _engine->getEntityManager().getEntities()) {
			if (entity && entity->getName() == name)
				return entity.get();
		}

		return nullptr;
	}

} // namespace Nawia::Entity
