#pragma once

#include <InteractiveTrigger.h>

#include <json.hpp>

namespace Nawia::Core { class Engine; }

namespace Nawia::Entity {

	class StoryTrigger : public InteractiveTrigger {
	public:
		StoryTrigger(
			const std::string& name,
			float x,
			float y,
			float width,
			float height,
			Core::Engine* engine,
			nlohmann::json data
		);

		void onTriggerEnter(Entity& other) override;
		void render(const Camera3D& camera) override;
		float getInteractionRange() override;
		[[nodiscard]] bool shouldWakeOnLocationChange() const override { return !(_once && _completed); }
		[[nodiscard]] nlohmann::json serializeState() const override;
		void applyState(const nlohmann::json& state, Item::ItemDatabase* item_database = nullptr) override;

	private:
		void run(Core::Engine* engine);
		void executeActions(Core::Engine* engine);

		Core::Engine* _engine = nullptr;
		nlohmann::json _data;
		bool _dialogue_open = false;
		bool _completed = false;
		bool _once = true;
	};

} // namespace Nawia::Entity
