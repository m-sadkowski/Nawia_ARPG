#pragma once

#include <SimpleMeleeEnemy.h>

#include <memory>

namespace Nawia::Core { class Engine; }

namespace Nawia::Entity {

	class Frog : public SimpleMeleeEnemy {
	public:
		void update(float dt) override;
		void setEngine(Core::Engine* engine) { _engine = engine; }

	protected:
		void onDeathStarted() override;
		void onAttackDamageApplied(Entity& target) override;

	private:
		Frog();
		friend class FrogBuilder;

		Core::Engine* _engine = nullptr;
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
