#include "MiniMushroomProp.h"

namespace Nawia::Entity {

	namespace {
		constexpr const char* MODEL_PATH = "assets/models/actors/mini_mushroom/mini_mushroom.glb";
	}

	MiniMushroomProp::MiniMushroomProp()
		: Entity("Gzibek", 0.0f, 0.0f, nullptr, 1)
	{
		_type = EntityType::NPCStatic;
		setFaction(Faction::None);
		setScale(0.3f);
		loadModel(MODEL_PATH);
		addAnimation("idle", MODEL_PATH, 4);
		addAnimation("jump", MODEL_PATH, 5);
		playAnimation("idle", true, false, 0, true);
	}

	void MiniMushroomProp::update(const float delta_time) {
		if (isDormant())
			return;

		Entity::update(delta_time);

		if (_jumping) {
			if (!isAnimationLocked()) {
				_jumping = false;
				_jump_timer = static_cast<float>(GetRandomValue(220, 520)) / 100.0f;
				playAnimation("idle");
			}
			return;
		}

		_jump_timer -= delta_time;
		if (_jump_timer <= 0.0f) {
			_jumping = true;
			playAnimation("jump", false, true, 0, true);
		}
	}

} // namespace Nawia::Entity
