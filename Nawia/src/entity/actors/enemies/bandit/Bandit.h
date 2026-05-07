#pragma once

#include <EnemyInterface.h>

#include <memory>

namespace Nawia::Entity {

	/**
	 * @class Bandit
	 * @brief Dystansowy wróg rzucający nożami.
	 *
	 * Bandyta utrzymuje preferowany dystans, cofa się gdy cel podejdzie zbyt
	 * blisko i odpala rzut nożem dopiero w odpowiedniej klatce animacji.
	 */
	class Bandit : public EnemyInterface {
	public:
		/** @brief Aktualizuje stan AI i animacji bandyty. */
		void update(float dt) override;

		/** @brief Obsługuje obrażenia oraz przejście do animacji trafienia. */
		void takeDamage(int dmg) override;

	private:
		Bandit();
		friend class BanditBuilder;

		enum class State {
			Idle,
			Chasing,
			Casting,
			GettingHit
		};

		State _state = State::Idle;
		State _state_before_hit = State::Idle;

		// Statystyki walki.
		static constexpr float VISION_RANGE = 14.0f;
		static constexpr float ATTACK_RANGE = 9.0f; // Preferowany dystans ataku.
		static constexpr float MIN_DISTANCE = 5.0f; // Minimalny dystans, który bandyta próbuje utrzymać.
		static constexpr float SPEED = 2.0f;
		static constexpr float KNIFE_COOLDOWN = 3.0f;

		// Wyznaczanie ścieżki przy odwrocie.
		static constexpr float PATH_RECALC_INTERVAL = 0.3f;
		float _knife_cooldown_timer = 0.0f;
		bool _is_retreating = false;
		bool _knife_thrown_this_cast = false; // Czy nóż został już rzucony w obecnej sekwencji.

		// Obsługa stanów.
		void handleIdleState(float dt);
		void handleChasingState(float dt);
		void handleCastingState(float dt);
		void handleGettingHitState(float dt);
		void onDeathStarted() override;
	};

	/**
	 * @class BanditBuilder
	 * @brief Builder konfigurujący instancję `Bandit`.
	 */
	class BanditBuilder : public EnemyBuilder<BanditBuilder> {
	public:
		/** @brief Tworzy roboczą instancję bandyty. */
		BanditBuilder() {
			_bandit_ptr = std::unique_ptr<Bandit>(new Bandit());
			this->_entity = _bandit_ptr.get();
		}

		/** @brief Oddaje gotową instancję bandyty. */
		std::unique_ptr<Bandit> build() {
			return std::move(_bandit_ptr);
		}

	private:
		std::unique_ptr<Bandit> _bandit_ptr;
	};

} // namespace Nawia::Entity
