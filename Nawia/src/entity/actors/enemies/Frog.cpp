#include "Frog.h"

#include <StoryNpc.h>

#include <memory>

namespace Nawia::Entity {

	namespace {
		constexpr const char* MODEL_PATH = "assets/models/frog.glb";
	}

	Frog::Frog() {
		setScale(1.0f);
		loadModel(MODEL_PATH);
		addAnimation("attack", MODEL_PATH, 0);
		addAnimation("death", MODEL_PATH, 1);
		addAnimation("idle", MODEL_PATH, 2);
		addAnimation("walk", MODEL_PATH, 3);
		configureAnimations("idle", "walk", "attack");
		configureCombat(12.0f, 1.35f, 2.8f, 22, 1.25f, 1.15f, 0.42f);
		playAnimation("idle", true, false, 0, true);
	}

	void Frog::onDeathStarted() {
		if (!_engine)
			return;

		auto village_head = std::make_shared<StoryNpc>("Soltys", getX(), getY());
		village_head->setAltitude(getAltitude());
		village_head->setAudioManager(_audio_manager);
		village_head->configureVillageHead(_engine);
		addPendingSpawn(village_head);
	}

} // namespace Nawia::Entity
