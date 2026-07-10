#include "SzeptuchaNpc.h"

#include <raymath.h>

#include <cmath>

namespace Nawia::Entity {

	namespace {
		constexpr const char* BABA_YAGA_MODEL = "assets/models/actors/baba_yaga/baba_yaga.glb";
		constexpr float BABA_YAGA_TARGET_HEIGHT = 2.0f;
		constexpr float BABA_YAGA_BOB_AMPLITUDE = 0.08f;
		constexpr float BABA_YAGA_BOB_SPEED = 1.25f;
		constexpr float BABA_YAGA_TILT_DEGREES = 2.8f;
		constexpr float BABA_YAGA_TILT_SPEED = 0.9f;
	}

	SzeptuchaNpc::SzeptuchaNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine)
		: StoryNpc(name, x, y, engine)
	{
		setType(EntityType::NPCStatic);
		setDialogueStageKey("szeptucha");
		replaceModel(BABA_YAGA_MODEL, false);

		if (fitLoadedModelToHeight(BABA_YAGA_TARGET_HEIGHT)) {
			setAltitude(0.0f);
			_base_model_transform = getModel().transform;
			_has_base_model_transform = true;
			updateIdleVisualTransform();
		}

		setPlaceholderDialogue("Szeptucha", "...");
	}

	void SzeptuchaNpc::update(const float delta_time) {
		if (isDormant())
			return;

		StoryNpc::update(delta_time);
		_idle_visual_time += delta_time;
		updateIdleVisualTransform();
	}

	Vector3 SzeptuchaNpc::getWorldPos3D() const {
		return {getX(), getAltitude() + getIdleBobOffset(), getY()};
	}

	float SzeptuchaNpc::getIdleBobOffset() const {
		return std::sin(_idle_visual_time * BABA_YAGA_BOB_SPEED) * BABA_YAGA_BOB_AMPLITUDE;
	}

	void SzeptuchaNpc::updateIdleVisualTransform() {
		if (!_has_base_model_transform || !hasModelLoaded())
			return;

		const float tilt = std::sin(_idle_visual_time * BABA_YAGA_TILT_SPEED) * BABA_YAGA_TILT_DEGREES * DEG2RAD;
		getModel().transform = MatrixMultiply(_base_model_transform, MatrixRotateX(tilt));
	}

} // namespace Nawia::Entity
