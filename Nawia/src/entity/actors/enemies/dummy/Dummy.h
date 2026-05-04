#pragma once

#include <EnemyInterface.h>

#include <memory>

namespace Nawia::Entity {

	/**
	 * @class Dummy
	 * @brief Testowy przeciwnik patrolujący losowe punkty i rzucający ognistą kulę.
	 */
	class Dummy : public EnemyInterface {
	public:
		/** @brief Tworzy testowego przeciwnika z podanymi statystykami. */
		Dummy(float x, float y, const std::shared_ptr<Texture2D>& tex, int max_hp, Core::Map* map);

		/** @brief Aktualizuje patrol, sekwencję rzutu i animacje. */
		void update(float dt) override;

	private:
		float _stay_timer = 0.0f;
		float _fireball_cooldown_timer = 0.0f;
		bool _is_casting = false;

		/** @brief Losuje nowy punkt patrolu na walkowalnym fragmencie mapy. */
		void pickNewTarget();

		/** @brief Obsługuje oczekiwanie na koniec animacji rzutu. */
		void handleCastingState(float dt);

		/** @brief Obsługuje patrol, ruch i rozpoczęcie rzutu. */
		void handleActiveState(float dt);
	};

} // namespace Nawia::Entity
