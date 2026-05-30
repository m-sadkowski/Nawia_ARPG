#pragma once

#include <StoryNpc.h>

namespace Nawia::Entity {

	class WandaCorpseNpc : public StoryNpc {
	public:
		WandaCorpseNpc(const std::string& name, float x, float y, Core::Engine* engine);
		[[nodiscard]] const char* getNpcClass() const override { return "wanda_corpse"; }
	};

} // namespace Nawia::Entity
