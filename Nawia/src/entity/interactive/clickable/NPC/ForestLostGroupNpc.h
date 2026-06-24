#pragma once

#include <StoryNpc.h>

#include <json.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Nawia::Entity {

	class ForestLostGroupNpc : public StoryNpc {
	public:
		ForestLostGroupNpc(
			const std::string& name,
			float x,
			float y,
			Core::Engine* engine,
			const nlohmann::json& data);
		~ForestLostGroupNpc() override;

		[[nodiscard]] const char* getNpcClass() const override { return "forest_lost_group"; }
		void onInteract(Entity& instigator) override;
		[[nodiscard]] bool canInteract() const override;
		void update(float delta_time) override;
		void render(const Camera3D& camera) override;
		void handleQuestTalkCompleted(Core::Engine& engine) override;

		[[nodiscard]] nlohmann::json serializeState() const override;
		void applyState(const nlohmann::json& state, Item::ItemDatabase* item_database = nullptr) override;

	private:
		class ForestGroupVisual;

		enum class CarryState {
			Waiting,
			Carrying,
			SisterDropping,
			SisterStandingUp,
			Dispersing,
			Arrived
		};

		struct HubDestination {
			Vector2 center = {0.0f, 0.0f};
			float radius = 5.0f;
		};

		struct CarryTuning {
			float spacing = 0.72f;
			float sister_bob_height = 0.045f;
			float sister_carry_height = 1.6f;
			float sister_drop_duration = 0.6f;
			float male_spacing_multiplier = 6.0f;
		};

		struct AnimationIndices {
			int death = 0;
			int idle = 9;
			int walk = 16;
			int walk_back = 17;
		};

		void configureFromJson(const nlohmann::json& data);
		void initializeMembers();
		void snapGroupToNavmesh();
		void loadGroupModelAndAnimations(Entity& entity, const std::string& model_path) const;
		void freezeSisterOnDeathFrame();
		void playIdle(Entity& entity) const;
		void playWalk(Entity& entity) const;
		void playWalkBack(Entity& entity) const;
		void startCarryRoute(Core::Engine& engine);
		[[nodiscard]] std::optional<HubDestination> resolveHub(Core::Engine& engine) const;
		void buildPathToPoint(Vector2 target);
		void trimCurrentPathStart();
		void updatePathMovement(float delta_time);
		void stopPathMovement();
		void updateCarrying(float delta_time);
		void updateCarryMovement(float delta_time);
		void updateCarryFormation(float delta_time);
		void startSisterDrop(const HubDestination& hub);
		void updateSisterDrop(float delta_time);
		void startSisterStandUp();
		void finishSisterStandUpIfReady();
		void startDispersal();
		void updateDispersal(float delta_time);
		void finishArrival();
		void promoteMainEntityToMilenaSister(bool adopt_sister_position = true);
		[[nodiscard]] Vector2 randomPointInHub(const HubDestination& hub) const;
		void snapMembersToFormation(Vector2 direction);
		void faceAlongDirection(Entity& entity, Vector2 direction) const;
		[[nodiscard]] Vector2 currentTravelDirection() const;
		void updateMemberAnimations(float delta_time);

		std::unique_ptr<ForestGroupVisual> _male_carrier;
		std::unique_ptr<ForestGroupVisual> _milena_sister;

		std::string _dialogue_key = "forest_lost_group";
		std::string _hub_name = "Herbalist Hub";
		std::string _checkpoint_on_arrival;
		std::string _start_quest_id;
		std::string _complete_quest_id;
		float _hub_radius_fallback = 5.0f;
		float _stop_distance = 0.65f;
		float _sister_bob_time = 0.0f;
		float _sister_drop_timer = 0.0f;
		float _sister_drop_start_altitude = 0.0f;
		CarryTuning _tuning;
		AnimationIndices _animation_indices;
		bool _talk_completed = false;
		bool _path_requested = false;
		bool _sister_is_standing = false;
		bool _main_is_milena_sister = false;
		CarryState _state = CarryState::Waiting;
		HubDestination _arrival_hub;
		Vector2 _destination = {0.0f, 0.0f};
		Vector2 _last_travel_direction = {1.0f, 0.0f};
		std::vector<Vector2> _current_path;
	};

} // namespace Nawia::Entity
