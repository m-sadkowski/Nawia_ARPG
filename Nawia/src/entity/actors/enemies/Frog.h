#pragma once

#include <SimpleMeleeEnemy.h>

namespace Nawia::Core { class Engine; }

namespace Nawia::Entity {

	class Frog : public SimpleMeleeEnemy {
	public:
		Frog();
		void setEngine(Core::Engine* engine) { _engine = engine; }

	protected:
		void onDeathStarted() override;

	private:
		Core::Engine* _engine = nullptr;
	};

} // namespace Nawia::Entity
