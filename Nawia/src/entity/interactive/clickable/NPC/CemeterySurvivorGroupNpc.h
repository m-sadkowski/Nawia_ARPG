#pragma once

#include <StoryNpc.h>

#include <json.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Nawia::Entity {

	class CemeterySurvivorGroupNpc : public StoryNpc {
	public:
		CemeterySurvivorGroupNpc(
			const std::string& name,
			float x,
			float y,
			Core::Engine* engine,
			const nlohmann::json& data);
		~CemeterySurvivorGroupNpc() override;

		[[nodiscard]] const char* getNpcClass() const override { return "cemetery_survivor_group"; }
		void onInteract(Entity& instigator) override;
		[[nodiscard]] bool canInteract() const override;
		void update(float delta_time) override;
		void render(const Camera3D& camera) override;
		void handleQuestTalkCompleted(Core::Engine& engine) override;
		[[nodiscard]] nlohmann::json serializeState() const override;
		void applyState(const nlohmann::json& state, Item::ItemDatabase* item_database = nullptr) override;

	private:
		class GroupVisual;

		struct HubDestination {
			Vector2 center = {0.0f, 0.0f};
			float radius = 5.0f;
		};

		void configureFromJson(const nlohmann::json& data);
		void initializeMaleSurvivor();
		void loadModelAndAnimations(Entity& entity, const std::string& model_path) const;
		void playIdle(Entity& entity) const;
		void playWalk(Entity& entity) const;
		void startRoute(Core::Engine& engine);
		[[nodiscard]] std::optional<HubDestination> resolveHub(Core::Engine& engine) const;
		void buildPathToPoint(Vector2 target);
		void trimCurrentPathStart();
		void updatePathMovement(float delta_time);
		void updateRoute(float delta_time);
		void startDispersal(const HubDestination& hub);
		void updateDispersal(float delta_time);
		void finishArrival();
		[[nodiscard]] Vector2 randomPointInHub(const HubDestination& hub) const;
		void stopPathMovement();
		void snapToNavmesh();

		std::unique_ptr<GroupVisual> _male_survivor;
		std::string _dialogue_key = "cemetery_survivors";
		std::string _hub_name = "Herbalist Hub";
		std::string _checkpoint_on_arrival = "cemetery_survivors_arrived";
		float _hub_radius_fallback = 5.0f;
		float _stop_distance = 0.65f;
		int _idle_animation_index = 9;
		int _walk_animation_index = 16;
		bool _talk_completed = false;
		bool _walking_to_hub = false;
		bool _dispersing = false;
		bool _arrived = false;
		bool _path_requested = false;
		Vector2 _destination = {0.0f, 0.0f};
		HubDestination _arrival_hub;
		std::vector<Vector2> _current_path;
	};

} // namespace Nawia::Entity
