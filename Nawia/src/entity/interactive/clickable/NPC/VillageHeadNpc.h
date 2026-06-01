#pragma once

#include <StoryNpc.h>

#include <vector>

namespace Nawia::Entity {

	class VillageHeadNpc : public StoryNpc {
	public:
		VillageHeadNpc(const std::string& name, float x, float y, Core::Engine* engine);
		[[nodiscard]] const char* getNpcClass() const override { return "village_head"; }
		void onInteract(Entity& instigator) override;
		[[nodiscard]] bool isMouseOver(float screen_x, float screen_y, const Camera3D& camera) const override;
		[[nodiscard]] bool canInteract() const override;
		void update(float delta_time) override;
		void handleQuestTalkCompleted(Core::Engine& engine) override;

	private:
		void startRouteToPlayerRespawn();
		void updateRouteToDestination(float delta_time);
		void buildPathToPoint(Vector2 target);
		void trimCurrentPathStart();
		void updatePathMovement(float delta_time);
		void stopPathMovement();

		bool _survivor_quest_started = false;
		bool _walking_to_spawn = false;
		bool _path_requested = false;
		Vector2 _destination = {0.0f, 0.0f};
		std::vector<Vector2> _current_path;
	};

} // namespace Nawia::Entity
