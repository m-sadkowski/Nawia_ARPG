#pragma once

#include <Dialogue.h>
#include <InteractiveClickable.h>

#include <string>
#include <utility>
#include <vector>

namespace Nawia::Core { class Engine; }

namespace Nawia::Entity {

	class StoryNpc : public InteractiveClickable {
	public:
		StoryNpc(const std::string& name, float x, float y, Core::Engine* engine = nullptr);
		[[nodiscard]] virtual const char* getNpcClass() const = 0;

		void onInteract(Entity& instigator) override;
		void onInteractionCompleted(Entity& instigator, Core::Engine& engine) override;
		[[nodiscard]] bool canInteract() const override;
		void update(float delta_time) override;
		float getInteractionRange() override;

		[[nodiscard]] const Game::DialogueTree& getDialogueTree() const { return _dialogue_tree; }
		void setDialogue(const Game::DialogueTree& dialogue) { _dialogue_tree = dialogue; }
		[[nodiscard]] int getDialogueStartNode() const { return _dialogue_resume_node; }
		[[nodiscard]] virtual bool shouldNotifyQuestTalkOnDialogueComplete() const;
		virtual void handleQuestTalkCompleted(Core::Engine& engine);
		void onDialogueClosed(int node_id, bool completed);

	protected:
		struct DialogueLine {
			std::string speaker;
			std::string text;
			std::string voice_path;
		};

		void setEngine(Core::Engine* engine) { _engine = engine; }
		void setDialogueStageKey(std::string key);
		[[nodiscard]] const std::string& getDialogueStageKey() const { return _dialogue_stage_key; }
		[[nodiscard]] const std::string& getLastCompletedDialogueStage() const { return _last_completed_dialogue_stage; }
		void setPlaceholderDialogue(const std::string& speaker, const std::string& text);
		[[nodiscard]] Game::DialogueTree buildLinearDialogue(
			const std::vector<std::pair<std::string, std::string>>& lines,
			const std::string& final_option_text) const;
		[[nodiscard]] Game::DialogueTree buildVoicedLinearDialogue(
			const std::vector<DialogueLine>& lines,
			const std::string& final_option_text) const;
		[[nodiscard]] Game::DialogueTree buildDialogueFromConfig(const std::string& key) const;

		Core::Engine* _engine = nullptr;
		bool _playing_talk = false;

	private:
		Game::DialogueTree _dialogue_tree;
		int _dialogue_resume_node = 0;
		std::string _dialogue_stage_key;
		std::string _last_completed_dialogue_stage;
	};

} // namespace Nawia::Entity
