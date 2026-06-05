#pragma once

#include <Dialogue.h>
#include <InteractiveClickable.h>

#include <string>
#include <utility>
#include <vector>

namespace Nawia::Core { class Engine; }

namespace Nawia::Entity {

	/**
	 * @brief Baza klikalnych NPC sterowanych konfiguracja dialogow.
	 *
	 * StoryNpc trzyma aktywne drzewo dialogowe oraz stan wznawiania etapow.
	 * Konkretne klasy NPC decyduja, kiedy podmienic dialog, ruszyc sie albo
	 * powiadomic questy.
	 */
	class StoryNpc : public InteractiveClickable {
	public:
		StoryNpc(const std::string& name, float x, float y, Core::Engine* engine = nullptr);
		/** @brief Stabilny identyfikator do wczytywania konfiguracji dialogu NPC. */
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
		/** @brief Odbiera koncowy wezel dialogu po zamknieciu DialogueUI. */
		void onDialogueClosed(int node_id, bool completed);

	protected:
		/** @brief Surowa linia dialogu z opcjonalna sciezka glosu. */
		struct DialogueLine {
			std::string speaker;
			std::string text;
			std::string voice_path;
		};

		void setEngine(Core::Engine* engine) { _engine = engine; }
		/** @brief Wybiera nazwany etap dialogu dla tego NPC. */
		void setDialogueStageKey(std::string key);
		[[nodiscard]] const std::string& getDialogueStageKey() const { return _dialogue_stage_key; }
		[[nodiscard]] const std::string& getLastCompletedDialogueStage() const { return _last_completed_dialogue_stage; }
		void setPlaceholderDialogue(const std::string& speaker, const std::string& text);
		/** @brief Buduje prosty lancuch gracz/NPC, gdy dialog JSON nie jest potrzebny. */
		[[nodiscard]] Game::DialogueTree buildLinearDialogue(
			const std::vector<std::pair<std::string, std::string>>& lines,
			const std::string& final_option_text) const;
		/** @brief Buduje liniowy dialog i zachowuje sciezki glosu dla kazdej linii. */
		[[nodiscard]] Game::DialogueTree buildVoicedLinearDialogue(
			const std::vector<DialogueLine>& lines,
			const std::string& final_option_text) const;
		/** @brief Wczytuje etap dialogu NPC ze wspolnej konfiguracji dialogow. */
		[[nodiscard]] Game::DialogueTree buildDialogueFromConfig(const std::string& key) const;

		Core::Engine* _engine = nullptr; ///< Nieposiadany dostep do systemow gry.
		bool _playing_talk = false;      ///< Prawda, gdy TALK ma nadpisac idle/walk.

	private:
		Game::DialogueTree _dialogue_tree; ///< Dialog aktualnie oferowany przez NPC.
		int _dialogue_resume_node = 0;     ///< Wezel ponownego otwarcia po zmianie etapu.
		std::string _dialogue_stage_key;   ///< Klucz konfiguracji aktywnego etapu fabuly.
		std::string _last_completed_dialogue_stage;
	};

} // namespace Nawia::Entity
