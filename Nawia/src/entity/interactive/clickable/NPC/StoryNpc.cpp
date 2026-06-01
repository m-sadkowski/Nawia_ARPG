#include "StoryNpc.h"

#include <Engine.h>
#include <Logger.h>
#include <UIHandler.h>

#include <fstream>
#include <json.hpp>

namespace Nawia::Entity {

	namespace {
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
		engine.getUIHandler().openDialogue(
			getDialogueTree(),
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

} // namespace Nawia::Entity
