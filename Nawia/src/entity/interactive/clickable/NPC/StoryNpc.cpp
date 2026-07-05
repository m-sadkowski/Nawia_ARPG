#include "StoryNpc.h"

#include <Engine.h>
#include <Logger.h>
#include <Player.h>
#include <UIHandler.h>

#include <fstream>
#include <json.hpp>
#include <raymath.h>

namespace Nawia::Entity {

	namespace {
		constexpr float LOOK_AT_PLAYER_INTERVAL = 0.25f;
		constexpr float LOOK_AT_PLAYER_RANGE = 10.0f;

		bool isPlayerDialogueSpeaker(const std::string& speaker) {
			return speaker == "Logos" || speaker == "Jarko" || speaker == "Player" || speaker == "Gracz";
		}

		bool isPlaceholderOption(const std::string& text) {
			return text.empty() || text == "..." || text == "Dalej";
		}

		std::string resolveFinalOption(const std::string& configured_text, const std::string& current_speaker, const std::string& current_text) {
			if (!isPlaceholderOption(configured_text))
				return configured_text;

			return isPlayerDialogueSpeaker(current_speaker) ? current_text : "Rozumiem.";
		}

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

		std::vector<std::string> readActionIds(const nlohmann::json& option_json) {
			std::vector<std::string> actions;
			if (option_json.contains("action") && option_json["action"].is_string())
				actions.push_back(option_json["action"].get<std::string>());

			if (option_json.contains("actions") && option_json["actions"].is_array()) {
				for (const auto& action_json : option_json["actions"]) {
					if (action_json.is_string())
						actions.push_back(action_json.get<std::string>());
				}
			}

			return actions;
		}
	}

	StoryNpc::StoryNpc(const std::string& name, const float x, const float y, Core::Engine* engine)
		: InteractiveClickable(name, x, y, nullptr, 1)
	{
		_engine = engine;
		setType(EntityType::NPCStatic);
		setFaction(Faction::None);
	}

	void StoryNpc::onInteract(Entity& instigator) {
		rotateTowardsCenter(instigator.getCenter().x, instigator.getCenter().y);
		instigator.rotateTowardsCenter(getCenter().x, getCenter().y);

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

	void StoryNpc::onInteractionCompleted(Entity& instigator, Core::Engine& engine) {
		(void)instigator;

		const auto self = std::dynamic_pointer_cast<StoryNpc>(shared_from_this());
		if (!self)
			return;

		std::weak_ptr<StoryNpc> npc_ref = self;
		engine.getUIHandler().openDialogueFacing(
			getDialogueTree(),
			self,
			getDialogueStartNode(),
			[npc_ref, engine_ptr = &engine](const int node_id, const bool completed) {
				const auto npc = npc_ref.lock();
				if (!npc || !engine_ptr)
					return;

				npc->onDialogueClosed(node_id, completed);
				if (!completed)
					return;

				if (npc->shouldNotifyQuestTalkOnDialogueComplete()) {
					engine_ptr->getQuestManager().notifyNPCTalked(npc->getName());
					engine_ptr->getQuestManager().update(engine_ptr);
				}
				npc->handleQuestTalkCompleted(*engine_ptr);
			});
	}

	bool StoryNpc::canInteract() const {
		return true;
	}

	void StoryNpc::update(const float delta_time) {
		if (isDormant())
			return;

		Entity::update(delta_time);
		if (isAnimationLocked())
			return;

		if (_playing_talk && (!_engine || !_engine->getUIHandler().isDialogueOpen())) {
			_playing_talk = false;
			if (getAnimationFrameCount("Idle") > 0)
				playAnimation("Idle", true, false, 0, true);
			else if (getAnimationFrameCount("idle") > 0)
				playAnimation("idle", true, false, 0, true);
		}

		if (!_playing_talk && !isMoving() && canInteract() && shouldLookAtPlayerWhenNearby())
			rotateToNearbyPlayer(delta_time);
	}

	float StoryNpc::getInteractionRange() {
		return 2.4f * 2.4f;
	}

	bool StoryNpc::shouldNotifyQuestTalkOnDialogueComplete() const {
		return true;
	}

	void StoryNpc::handleQuestTalkCompleted(Core::Engine& engine) {
		(void)engine;
	}

	void StoryNpc::onDialogueClosed(const int node_id, const bool completed) {
		_last_completed_dialogue_stage = completed ? _dialogue_stage_key : "";
		_dialogue_resume_node = completed ? 0 : node_id;
	}

	void StoryNpc::setDialogueStageKey(std::string key) {
		if (_dialogue_stage_key == key)
			return;

		_dialogue_stage_key = std::move(key);
		_dialogue_resume_node = 0;
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
			if (next_line < lines.size() && isPlayerDialogueSpeaker(lines[next_line].first)) {
				option.text = lines[next_line].second;
			} else {
				const bool is_final_node = next_line >= lines.size();
				if (is_final_node)
					option.text = resolveFinalOption(final_option_text, lines[i].first, lines[i].second);
				else
					option.text = "Dalej";
			}

			option.next_node_id = (next_line < lines.size()) ? static_cast<int>(next_line) : -1;
			node.options.push_back(option);
			tree.addNode(node);

			i++;
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
			size_t next_line = i + 1;
			if (next_line < lines.size() && isPlayerDialogueSpeaker(lines[next_line].speaker)) {
				option.text = lines[next_line].text;
			} else {
				const bool is_final_node = next_line >= lines.size();
				if (is_final_node)
					option.text = resolveFinalOption(final_option_text, lines[i].speaker, lines[i].text);
				else
					option.text = "Dalej";
			}
			option.next_node_id = (next_line < lines.size()) ? static_cast<int>(next_line) : -1;
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
		const std::string final_option = dialogue.value("final_option", "Rozumiem.");
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

	Game::DialogueTree StoryNpc::buildDialogueFromConfig(
		const std::string& key,
		const std::function<void(const std::string&)>& execute_option_action) const {
		const auto& config = getNpcDialogueConfig();
		if (!config.contains(key) || !config[key].is_object())
			return {};

		const auto& dialogue = config[key];
		const auto nodes_it = dialogue.find("nodes");
		if (nodes_it == dialogue.end() || !nodes_it->is_array())
			return buildDialogueFromConfig(key);

		Game::DialogueTree tree;
		for (const auto& node_json : *nodes_it) {
			if (!node_json.is_object())
				continue;

			Game::DialogueNode node;
			node.id = node_json.value("id", 0);
			node.speaker_name = node_json.value("speaker", "");
			node.text = node_json.value("text", "");
			node.voice_path = node_json.value("voice_path", "");

			if (node_json.contains("options") && node_json["options"].is_array()) {
				for (const auto& option_json : node_json["options"]) {
					if (!option_json.is_object())
						continue;

					Game::DialogueOption option;
					option.text = option_json.value("text", "Dalej");
					option.next_node_id = option_json.value("next_node_id", option_json.value("next", -1));

					auto actions = readActionIds(option_json);
					if (!actions.empty() && execute_option_action) {
						option.action = [actions = std::move(actions), execute_option_action]() {
							for (const auto& action : actions)
								execute_option_action(action);
						};
					}

					node.options.push_back(std::move(option));
				}
			}

			if (node.options.empty()) {
				Game::DialogueOption option;
				option.text = node_json.value("option", "Rozumiem.");
				option.next_node_id = -1;
				node.options.push_back(std::move(option));
			}

			tree.addNode(node);
		}

		return tree;
	}

	void StoryNpc::rotateToNearbyPlayer(const float delta_time) {
		if (!_engine)
			return;

		_look_at_player_timer -= delta_time;
		if (_look_at_player_timer > 0.0f)
			return;

		_look_at_player_timer = LOOK_AT_PLAYER_INTERVAL;
		const auto player = _engine->getPlayer();
		if (!player || player->isDead() || player->isDying())
			return;

		if (Vector2DistanceSqr(getCenter(), player->getCenter()) <= LOOK_AT_PLAYER_RANGE * LOOK_AT_PLAYER_RANGE)
			rotateTowardsCenter(player->getCenter().x, player->getCenter().y);
	}

} // namespace Nawia::Entity
