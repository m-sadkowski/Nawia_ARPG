#pragma once

#include <StoryNpc.h>

namespace Nawia::Entity {

	class VillageHeadNpc : public StoryNpc {
	public:
		VillageHeadNpc(const std::string& name, float x, float y, Core::Engine* engine);
		[[nodiscard]] const char* getNpcClass() const override { return "village_head"; }
	};

} // namespace Nawia::Entity
