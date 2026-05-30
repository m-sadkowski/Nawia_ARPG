#include "WandaCorpseNpc.h"

namespace Nawia::Entity {

	WandaCorpseNpc::WandaCorpseNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine)
		: StoryNpc(name, x, y)
	{
		configureWandaCorpse(engine);
	}

} // namespace Nawia::Entity
