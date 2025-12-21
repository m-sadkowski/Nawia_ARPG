#pragma once
#include "InteractiveObject.h"


namespace  Nawia::Entity
{
	class Trigger : public InteractiveObject
	{

		
		protected:
			float _width;
			float _height;

		public:
			virtual bool checkCollision(const std::shared_ptr<Entity>& target) const;
			
			Trigger(float start_x, float start_y,
				const std::shared_ptr<SDL_Texture>& texture,
				int max_hp, float tw, float th);
			

	};
}

