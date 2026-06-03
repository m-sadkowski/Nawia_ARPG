#include "SzeptuchaNpc.h"

namespace Nawia::Entity {

	namespace {
		constexpr const char* BABA_YAGA_MODEL = "assets/models/actors/baba_yaga/baba_yaga.glb";
		constexpr float BABA_YAGA_TARGET_HEIGHT = 2.0f;
	}

	SzeptuchaNpc::SzeptuchaNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine)
		: StoryNpc(name, x, y, engine)
	{
		_type = EntityType::NPCStatic;
		setDialogueStageKey("szeptucha");
		replaceModel(BABA_YAGA_MODEL, false);

		if (fitLoadedModelToHeight(BABA_YAGA_TARGET_HEIGHT)) {
			setAltitude(0.0f);
		}

		setPlaceholderDialogue("Szeptucha", "...");
	}

} // namespace Nawia::Entity
