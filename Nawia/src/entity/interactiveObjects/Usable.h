#pragma once
#include "InteractiveObject.h"

namespace Nawia::Entity
{
	class Usable : public InteractiveObject
	{
	public:
		using InteractiveObject::InteractiveObject;
		
		//[[nodiscard]] virtual float getInteractionRange() const override = 0;
		[[nodiscard]] bool isPlayerInRange(float player_x, float player_y) const;
	};
}
