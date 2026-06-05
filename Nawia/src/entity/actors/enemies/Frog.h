#pragma once

#include <SimpleMeleeEnemy.h>

#include <memory>

namespace Nawia::Core { class Engine; }

namespace Nawia::Entity {

	/**
	 * @brief Skoczny przeciwnik walczacy w zwarciu i cofajacy sie po trafieniu celu.
	 *
	 * Frog korzysta z bazowego AI `SimpleMeleeEnemy`, ale po udanym ataku wykonuje
	 * krotki odwrot po navmeshu. W wersji bossowej po smierci uwalnia Soltysa.
	 */
	class Frog : public SimpleMeleeEnemy {
	public:
		/** @brief Aktualizuje bazowe AI albo specjalny stan odwrotu po ataku. */
		void update(float dt) override;
		void setEngine(Core::Engine* engine) { _engine = engine; }

	protected:
		/** @brief Spawnuje NPC Soltysa po smierci ropucha-bossa. */
		void onDeathStarted() override;
		/** @brief Odtwarza feedback trafienia i uruchamia stan odwrotu. */
		void onAttackDamageApplied(Entity& target) override;

	private:
		Frog();
		friend class FrogBuilder;

		Core::Engine* _engine = nullptr;
		/** @brief Pozostaly czas ucieczki od celu po udanym ataku. */
		float _retreat_timer = 0.0f;
	};

	class FrogBuilder : public EnemyBuilder<FrogBuilder> {
	public:
		FrogBuilder() {
			_frog_ptr = std::unique_ptr<Frog>(new Frog());
			this->_entity = _frog_ptr.get();
		}

		FrogBuilder& setEngine(Core::Engine* engine) {
			_frog_ptr->_engine = engine;
			return *this;
		}

		std::unique_ptr<Frog> build() {
			return std::move(_frog_ptr);
		}

	private:
		std::unique_ptr<Frog> _frog_ptr;
	};

} // namespace Nawia::Entity
