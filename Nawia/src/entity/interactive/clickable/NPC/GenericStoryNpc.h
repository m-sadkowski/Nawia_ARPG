#pragma once

#include <StoryNpc.h>

#include <json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace Nawia::Entity {

	/**
	 * @brief Konfigurowalny fabularny NPC dla ludzi, zielarza i ocalencow.
	 */
	class GenericStoryNpc : public StoryNpc {
	public:
		GenericStoryNpc(
			const std::string& name,
			float x,
			float y,
			Core::Engine* engine,
			const nlohmann::json& data);

		[[nodiscard]] const char* getNpcClass() const override { return _npc_class_name.c_str(); }
		void onInteract(Entity& instigator) override;
		[[nodiscard]] bool canInteract() const override;
		void update(float delta_time) override;
		void render(const Camera3D& camera) override;
		void handleQuestTalkCompleted(Core::Engine& engine) override;

		[[nodiscard]] nlohmann::json serializeState() const override;
		void applyState(const nlohmann::json& state, Item::ItemDatabase* item_database = nullptr) override;

	private:
		void configureModel(const nlohmann::json& data);
		void configureDialogue(const nlohmann::json& data);
		void executeTalkActions(Core::Engine& engine);
		void startRoute(Core::Engine& engine);
		[[nodiscard]] std::optional<Vector2> resolveDestination(Core::Engine& engine) const;
		void updateRouteToDestination(float delta_time);
		void buildPathToPoint(Vector2 target);
		void trimCurrentPathStart();
		void updatePathMovement(float delta_time);
		void stopPathMovement();
		void playIdleAnimation();
		void playWalkAnimation();

		std::string _npc_class_name = "story_human";
		std::string _dialogue_key;
		std::string _model_path;
		std::string _animation_bundle_path;
		std::string _idle_animation = "Idle";
		std::string _walk_animation = "Walk";
		std::string _talk_animation = "talk";
		std::string _destination_name;
		std::string _start_quest_id;
		std::string _complete_quest_id;
		std::string _fail_quest_id;
		std::string _checkpoint_on_talk;
		std::string _checkpoint_on_arrival;
		std::optional<Vector2> _destination_position;

		float _stop_distance = 0.65f;
		bool _can_talk = true;
		bool _disable_interaction_after_talk = false;
		bool _route_after_talk = false;
		bool _hide_on_arrival = false;
		bool _talk_completed = false;
		bool _walking_to_destination = false;
		bool _path_requested = false;
		bool _arrived = false;
		Vector2 _destination = {0.0f, 0.0f};
		std::vector<Vector2> _current_path;
	};

} // namespace Nawia::Entity
