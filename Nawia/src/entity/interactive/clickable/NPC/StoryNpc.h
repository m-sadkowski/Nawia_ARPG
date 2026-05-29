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
		StoryNpc(const std::string& name, float x, float y);

		void configureMushroom(Core::Engine* engine, const std::string& follow_checkpoint_name = "Checkpoint Gziba");
		void configureVillageHead(Core::Engine* engine);

		void onInteract(Entity& instigator) override;
		void update(float delta_time) override;
		float getInteractionRange() override;

		[[nodiscard]] const Game::DialogueTree& getDialogueTree() const { return _dialogue_tree; }
		void setDialogue(const Game::DialogueTree& dialogue) { _dialogue_tree = dialogue; }
		[[nodiscard]] int getDialogueStartNode() const { return _dialogue_resume_node; }
		[[nodiscard]] bool shouldNotifyQuestTalkOnDialogueComplete() const;
		void handleQuestTalkCompleted(Core::Engine& engine);
		void onDialogueClosed(int node_id, bool completed);

	private:
		void setPlaceholderDialogue(const std::string& speaker, const std::string& text);
		void refreshMushroomDialogue();
		[[nodiscard]] Game::DialogueTree buildLinearDialogue(
			const std::vector<std::pair<std::string, std::string>>& lines,
			const std::string& final_option_text) const;
		void updateMushroomFollow(float delta_time);
		void buildPathToFollowCheckpoint(const Entity& checkpoint);
		void trimCurrentPathStart();
		void updatePathMovement(float delta_time);
		void stopPathMovement();
		void updateProceduralMushroomAnimation(float delta_time);
		void rotateToPlayerOnInterval(float delta_time);
		[[nodiscard]] int getRescuedMushroomCount() const;
		[[nodiscard]] int getRequiredRescueCount() const;
		[[nodiscard]] bool areAllMushroomsAlreadyRescued() const;
		Entity* findEntityByName(const std::string& name) const;

		Core::Engine* _engine = nullptr;
		Game::DialogueTree _dialogue_tree;
		std::string _follow_checkpoint_name;
		bool _can_follow = false;
		bool _reached_follow_checkpoint = false;
		bool _follow_path_requested = false;
		bool _playing_talk = false;
		bool _use_procedural_mushroom_animation = false;
		bool _procedural_base_altitude_initialized = false;
		int _dialogue_resume_node = 0;
		std::string _dialogue_stage_key;
		std::string _last_completed_dialogue_stage;
		float _procedural_anim_time = 0.0f;
		float _base_altitude = 0.0f;
		float _look_at_player_timer = 0.0f;
		std::vector<Vector2> _current_path;
	};

} // namespace Nawia::Entity
