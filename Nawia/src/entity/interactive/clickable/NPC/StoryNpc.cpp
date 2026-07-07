#include "StoryNpc.h"

#include <Engine.h>
#include <DialogueJson.h>
#include <Logger.h>
#include <Player.h>
#include <UIHandler.h>

#include <raymath.h>

namespace Nawia::Entity {

	namespace {
		constexpr float LOOK_AT_PLAYER_INTERVAL = 0.25f;
		constexpr float LOOK_AT_PLAYER_RANGE = 10.0f;

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
			if (next_line < lines.size() && Game::DialogueJson::isPlayerSpeaker(lines[next_line].first)) {
				option.text = lines[next_line].second;
			} else {
				const bool is_final_node = next_line >= lines.size();
				if (is_final_node)
					option.text = Game::DialogueJson::resolveFinalOption(final_option_text, lines[i].first, lines[i].second);
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
			if (next_line < lines.size() && Game::DialogueJson::isPlayerSpeaker(lines[next_line].speaker)) {
				option.text = lines[next_line].text;
			} else {
				const bool is_final_node = next_line >= lines.size();
				if (is_final_node)
					option.text = Game::DialogueJson::resolveFinalOption(final_option_text, lines[i].speaker, lines[i].text);
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
		const auto* dialogue = Game::DialogueJson::findNpcDialogue(key);
		return dialogue ? Game::DialogueJson::buildLinearTree(*dialogue) : Game::DialogueTree{};
	}

	Game::DialogueTree StoryNpc::buildDialogueFromConfig(
		const std::string& key,
		const std::function<void(const std::string&)>& execute_option_action) const {
		const auto* dialogue = Game::DialogueJson::findNpcDialogue(key);
		if (!dialogue)
			return {};

		const auto nodes_it = dialogue->find("nodes");
		if (nodes_it == dialogue->end() || !nodes_it->is_array())
			return buildDialogueFromConfig(key);

		return Game::DialogueJson::buildStringActionTree(*dialogue, execute_option_action);
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
