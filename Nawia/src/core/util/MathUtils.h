#pragma once

namespace Nawia::Core {

	class Point2D {
	public:
		Point2D(const float x, const float y) : _x(x), _y(y) {}

		static Point2D screenToIso(float mouse_x, float mouse_y, float offsetX, float offsetY);

		float getX() { return _x; }
		float getY() { return _y; }

		void setX(float x) { _x = x; }
		void setY(float y) { _y = y; }
	private:
		float _x, _y;
	};

} // namespace Nawia::Core