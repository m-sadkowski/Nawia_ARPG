#pragma once

#include <StoryNpc.h>

namespace Nawia::Entity {

	class SzeptuchaNpc : public StoryNpc {
	public:
		SzeptuchaNpc(const std::string& name, float x, float y, Core::Engine* engine);
		[[nodiscard]] const char* getNpcClass() const override { return "szeptucha"; }
	};

} // namespace Nawia::Entity
