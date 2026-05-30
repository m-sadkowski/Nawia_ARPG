#include "SzeptuchaNpc.h"

namespace Nawia::Entity {

	SzeptuchaNpc::SzeptuchaNpc(
		const std::string& name,
		const float x,
		const float y,
		Core::Engine* engine)
		: StoryNpc(name, x, y)
	{
		configureSzeptucha(engine);
	}

} // namespace Nawia::Entity
