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
		void configureSzeptucha(Core::Engine* engine);
		void configureWandaCorpse(Core::Engine* engine);

		void onInteract(Entity& instigator) override;
		[[nodiscard]] bool isMouseOver(float screen_x, float screen_y, const Camera3D& camera) const override;
		[[nodiscard]] bool canInteract() const override;
		void update(float delta_time) override;
		float getInteractionRange() override;

		[[nodiscard]] const Game::DialogueTree& getDialogueTree() const { return _dialogue_tree; }
		void setDialogue(const Game::DialogueTree& dialogue) { _dialogue_tree = dialogue; }
		[[nodiscard]] int getDialogueStartNode() const { return _dialogue_resume_node; }
		[[nodiscard]] bool isWandaCorpse() const { return _is_wanda_corpse; }
		[[nodiscard]] bool shouldNotifyQuestTalkOnDialogueComplete() const;
		void handleQuestTalkCompleted(Core::Engine& engine);
		void onDialogueClosed(int node_id, bool completed);

	private:
		enum class TravelMode {
			None,
			ToBrothers,
			ReturningHome
		};

		struct DialogueLine {
			std::string speaker;
			std::string text;
			std::string voice_path;
		};

		void setPlaceholderDialogue(const std::string& speaker, const std::string& text);
		void refreshMushroomDialogue();
		[[nodiscard]] Game::DialogueTree buildLinearDialogue(
			const std::vector<std::pair<std::string, std::string>>& lines,
			const std::string& final_option_text) const;
		[[nodiscard]] Game::DialogueTree buildVoicedLinearDialogue(
			const std::vector<DialogueLine>& lines,
			const std::string& final_option_text) const;
		void updateMushroomFollow(float delta_time);
		void updateVillageHeadTravel(float delta_time);
		void startVillageHeadRouteToPlayerSpawn();
		void startMushroomRoute(TravelMode mode);
		void advanceMushroomRoute();
		void buildPathToPoint(Vector2 target);
		void trimCurrentPathStart();
		void updatePathMovement(float delta_time);
		void stopPathMovement();
		void sendPurifiedMushroomsHome();
		void updateProceduralMushroomAnimation(float delta_time);
		void rotateToPlayerOnInterval(float delta_time);
		[[nodiscard]] std::vector<Vector2> collectOrderedFollowWaypoints(bool reverse_to_home) const;
		[[nodiscard]] int getRescuedMushroomCount() const;
		[[nodiscard]] int getRequiredRescueCount() const;
		[[nodiscard]] bool areAllMushroomsAlreadyRescued() const;
		[[nodiscard]] bool hasMushroomDialogueAvailable() const;
		Entity* findEntityByName(const std::string& name) const;

		Core::Engine* _engine = nullptr;
		Game::DialogueTree _dialogue_tree;
		std::string _follow_checkpoint_name;
		bool _can_follow = false;
		bool _reached_follow_checkpoint = false;
		bool _follow_path_requested = false;
		bool _return_started = false;
		bool _is_village_head = false;
		bool _is_wanda_corpse = false;
		bool _wanda_corpse_inspected = false;
		bool _survivor_quest_started = false;
		bool _village_head_walking_to_spawn = false;
		bool _playing_talk = false;
		bool _use_procedural_mushroom_animation = false;
		bool _procedural_base_altitude_initialized = false;
		int _dialogue_resume_node = 0;
		std::string _dialogue_stage_key;
		std::string _last_completed_dialogue_stage;
		float _procedural_anim_time = 0.0f;
		float _base_altitude = 0.0f;
		float _look_at_player_timer = 0.0f;
		float _mushroom_step_sound_timer = 0.0f;
		std::vector<Vector2> _current_path;
		std::vector<Vector2> _travel_waypoints;
		size_t _travel_waypoint_index = 0;
		TravelMode _travel_mode = TravelMode::None;
		Vector2 _home_position = {0.0f, 0.0f};
		Vector2 _village_head_destination = {0.0f, 0.0f};
	};

} // namespace Nawia::Entity
