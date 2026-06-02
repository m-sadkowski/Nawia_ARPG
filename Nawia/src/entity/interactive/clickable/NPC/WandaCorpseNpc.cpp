#include "WandaCorpseNpc.h"

#include <Engine.h>

#include <raymath.h>

namespace Nawia::Entity {

	namespace {
		constexpr const char* WANDA_CORPSE_MODEL = "assets/models/actors/wanda/woman_dress.glb";
	}

	WandaCorpseNpc::WandaCorpseNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine)
		: StoryNpc(name, x, y, engine)
	{
		_type = EntityType::NPCStatic;
		setDialogueStageKey("wanda_corpse");
		setFaction(Faction::None);
		replaceModel(WANDA_CORPSE_MODEL, false);
		if (hasModelLoaded()) {
			const BoundingBox bounds = GetModelBoundingBox(getModel());
			const float center_x = 0.5f * (bounds.min.x + bounds.max.x);
			const float center_z = 0.5f * (bounds.min.z + bounds.max.z);
			getModel().transform = MatrixMultiply(
				MatrixMultiply(
					MatrixTranslate(-center_x, -bounds.min.y + 0.14f, -center_z),
					MatrixRotateX(PI / 2.0f)),
				getModel().transform);
		}
		setScale(1.65f);
		setDialogue(buildDialogueFromConfig("wanda_corpse"));
	}

	void WandaCorpseNpc::onInteract(Entity& instigator) {
		instigator.rotateTowardsCenter(getCenter().x, getCenter().y);
	}

	bool WandaCorpseNpc::canInteract() const {
		return !_inspected;
	}

	float WandaCorpseNpc::getInteractionRange() {
		return 3.2f * 3.2f;
	}

	void WandaCorpseNpc::handleQuestTalkCompleted(Core::Engine& engine) {
		if (_inspected)
			return;

		_inspected = true;
		const Vector2 corpse_center = getCenter();
		engine.notifyStoryEvent("wanda_corpse_inspected", corpse_center);
	}

} // namespace Nawia::Entity
