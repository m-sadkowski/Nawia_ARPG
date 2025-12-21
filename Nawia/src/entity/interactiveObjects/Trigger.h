#pragma once
#include "InteractiveObject.h"


namespace  Nawia::Entity
{
	class Trigger : public InteractiveObject
	{
		public:
			virtual bool checkCollision(const std::shared_ptr<Entity>& target) const;

	};
}

