#pragma once

#include <SimpleMeleeEnemy.h>

namespace Nawia::Core { class Engine; }

namespace Nawia::Entity {

	class Frog : public SimpleMeleeEnemy {
	public:
		Frog();
		void update(float dt) override;
		void setEngine(Core::Engine* engine) { _engine = engine; }

	protected:
		void onDeathStarted() override;
		void onAttackDamageApplied(Entity& target) override;

	private:
		Core::Engine* _engine = nullptr;
		float _retreat_timer = 0.0f;
	};

} // namespace Nawia::Entity
