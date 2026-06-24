#pragma once

#include <StoryNpc.h>

#include <vector>

namespace Nawia::Entity {

	/**
	 * @brief Fabularny NPC, ktory moze opuscic wies i dojsc do huba zielarza.
	 *
	 * W odroznieniu od pasywnego StoryNpc soltys posiada krotka oskryptowana
	 * trase po starcie questa z ocalałymi.
	 */
	class VillageHeadNpc : public StoryNpc {
	public:
		VillageHeadNpc(const std::string& name, float x, float y, Core::Engine* engine);
		[[nodiscard]] const char* getNpcClass() const override { return "village_head"; }
		void onInteract(Entity& instigator) override;
		[[nodiscard]] bool isMouseOver(float screen_x, float screen_y, const Camera3D& camera) const override;
		[[nodiscard]] bool canInteract() const override;
		void update(float delta_time) override;
		void handleQuestTalkCompleted(Core::Engine& engine) override;
		[[nodiscard]] nlohmann::json serializeState() const override;
		void applyState(const nlohmann::json& state, Item::ItemDatabase* item_database = nullptr) override;

	private:
		void startRouteToHerbalistHub();
		void updateRouteToDestination(float delta_time);
		void buildPathToPoint(Vector2 target);
		void trimCurrentPathStart();
		void updatePathMovement(float delta_time);
		void stopPathMovement();

		bool _survivor_quest_started = false;
		bool _walking_to_spawn = false;
		bool _path_requested = false; ///< Pilnuje, zeby generowac sciezke raz na cel.
		Vector2 _destination = {0.0f, 0.0f};
		std::vector<Vector2> _current_path;
	};

} // namespace Nawia::Entity
