#include "Worm.h"

#include <MiniMushroomInfected.h>
#include <SoundIds.h>

namespace Nawia::Entity {

	namespace {
		constexpr const char* MODEL_PATH = "assets/models/actors/worm/worm.glb";
	}

	Worm::Worm() {
		setScale(0.3f);
		loadModel(MODEL_PATH);
		addAnimation("attack", MODEL_PATH, 0);
		addAnimation("idle", MODEL_PATH, 1);
		addAnimation("death", MODEL_PATH, 2);
		addAnimation("walk", MODEL_PATH, 3);
		configureAnimations("idle", "walk", "attack");
		configureCombat(11.0f, 1.0f, 2.4f, 7, 0.5f, 2.4f, 0.45f);
		playAnimation("idle", true, false, 0, true);
	}

	void Worm::setLinkedMushroom(const std::weak_ptr<MiniMushroomInfected>& mushroom) {
		_linked_mushroom = mushroom;
	}

	void Worm::update(const float dt) {
		SimpleMeleeEnemy::update(dt);
		updateMovementSound(Audio::SoundPath::WormMovement, _state == State::Chasing && _is_moving, 0.34f, 1.0f);
	}

	void Worm::onDeathStarted() {
		// Robal nie zna fabuly; tylko odblokowuje stan oczyszczenia w swojej skorupie.
		if (const auto mushroom = _linked_mushroom.lock())
			mushroom->purifyAfterWormDeath();
	}

} // namespace Nawia::Entity
