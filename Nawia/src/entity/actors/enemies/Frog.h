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
		/** @brief Renderuje model oraz czytelny telegraph/efekt jezyka. */
		void render(const Camera3D& camera) override;
		void setEngine(Core::Engine* engine) { _engine = engine; }

	protected:
		/** @brief Spawnuje NPC Soltysa po smierci ropucha-bossa. */
		void onDeathStarted() override;
		/** @brief Odtwarza feedback trafienia i uruchamia stan odwrotu. */
		void onAttackDamageApplied(Entity& target) override;

	private:
		Frog();
		friend class FrogBuilder;

		enum class SpecialState {
			None,
			TongueWindup,
			TonguePull,
			TongueRecover,
			SideHop,
			ToxicSpitWindup
		};

		void updateSpecialState(float dt);
		void updateTongueWindup(float dt);
		void updateTonguePull(float dt);
		void updateTongueRecover(float dt);
		void updateSideHop(float dt);
		void updateToxicSpitWindup(float dt);
		void tryStartSpecialMove();
		void startTongueStrike();
		void releaseTongueStrike();
		void startSideHop();
		void startToxicPool();
		void finishSpecialMove();
		void stopMoving();
		[[nodiscard]] bool isBossVariant() const;
		[[nodiscard]] bool isTargetInTongueLane(const Entity& target) const;
		[[nodiscard]] Vector2 getTongueAimDirection() const;
		[[nodiscard]] float randomRange(float min, float max) const;

		Core::Engine* _engine = nullptr;
		/** @brief Pozostaly czas ucieczki od celu po udanym ataku. */
		float _retreat_timer = 0.0f;
		SpecialState _special_state = SpecialState::None;
		float _special_timer = 0.0f;
		float _tongue_cooldown_timer = 1.6f;
		float _sidehop_cooldown_timer = 1.1f;
		float _toxic_pool_cooldown_timer = 2.4f;
		Vector2 _tongue_target_snapshot = {0.0f, 0.0f};
		Vector2 _toxic_pool_target_snapshot = {0.0f, 0.0f};
		Vector2 _sidehop_target = {0.0f, 0.0f};
		std::weak_ptr<Entity> _tongue_victim;
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
