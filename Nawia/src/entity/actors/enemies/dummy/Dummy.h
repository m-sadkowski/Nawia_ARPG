#pragma once

#include <EnemyInterface.h>

#include <memory>

namespace Nawia::Entity {

	/**
	 * @class Dummy
	 * @brief Testowy przeciwnik patrolujący losowe punkty i rzucający fireball.
	 */
	class Dummy : public EnemyInterface {
	public:
		/** @brief Tworzy testowego przeciwnika z podanymi statystykami. */
		Dummy(float x, float y, const std::shared_ptr<Texture2D>& tex, int max_hp, Core::Map* map);

		/** @brief Aktualizuje patrol, castowanie i animacje. */
		void update(float dt) override;

	private:
		float _stay_timer;
		float _fireball_cooldown_timer;
		bool _is_casting = false;

		void pickNewTarget();
		
		void handleCastingState(float dt);
		void handleActiveState(float dt);
	};

} // namespace Nawia::Entity
