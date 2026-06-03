#pragma once

#include <StoryNpc.h>

namespace Nawia::Entity {

	/**
	 * @brief Klikalne zwloki z intro, ktore bramkuja pierwsze sceny Wczora.
	 *
	 * Uzywa mechaniki dialogu StoryNpc, ale nie wysyla zakonczenia rozmowy do
	 * questow, bo sekwencja poziomu reaguje na wlasny event inspekcji zwlok.
	 */
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
		bool _inspected = false; ///< Chroni przed ponownym odpaleniem inspekcji zwlok.
	};

} // namespace Nawia::Entity
