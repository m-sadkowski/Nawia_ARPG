#pragma once

#include <StoryNpc.h>

namespace Nawia::Entity {

	class WandaCorpseNpc : public StoryNpc {
	public:
		WandaCorpseNpc(const std::string& name, float x, float y, Core::Engine* engine);
		[[nodiscard]] const char* getNpcClass() const override { return "wanda_corpse"; }
		[[nodiscard]] bool shouldNotifyQuestTalkOnDialogueComplete() const override { return false; }
		void onInteract(Entity& instigator) override;
		[[nodiscard]] bool canInteract() const override;
		float getInteractionRange() override;
		void handleQuestTalkCompleted(Core::Engine& engine) override;

	private:
		bool _inspected = false;
	};

} // namespace Nawia::Entity
