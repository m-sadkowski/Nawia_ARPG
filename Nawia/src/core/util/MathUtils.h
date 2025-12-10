#pragma once
#include "Map.h"

namespace Nawia::Core {

	class Point2D {
	public:
		Point2D(float x, float y)
			: _x(x), _y(y) { }

		static Point2D screenToIso(float mouseX, float mouseY);

		float getX() { return _x; }
		float getY() { return _y; }

		void setX(float x) { _x = x; }
		void setY(float y) { _y = y; }
	private:
		float _x, _y;
	};

}