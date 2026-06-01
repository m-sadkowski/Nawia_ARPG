#include "SzeptuchaNpc.h"

#include <raymath.h>

#include <algorithm>

namespace Nawia::Entity {

	namespace {
		constexpr const char* BABA_YAGA_MODEL = "assets/models/actors/szeptucha/baba_yaga.glb";
	}

	SzeptuchaNpc::SzeptuchaNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine)
		: StoryNpc(name, x, y)
	{
		setEngine(engine);
		_type = EntityType::NPCStatic;
		setDialogueStageKey("szeptucha");
		replaceModel(BABA_YAGA_MODEL, false);
		addAnimation("default", BABA_YAGA_MODEL, 0);
		//playAnimation("default"); // nie działa
		setScale(100.0f);

		setPlaceholderDialogue("Szeptucha", "...");
	}

} // namespace Nawia::Entity
