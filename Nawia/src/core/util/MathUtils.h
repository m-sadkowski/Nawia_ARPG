#pragma once

#include <raylib.h>

namespace Nawia::Core {

	/**
	 * @brief Przelicza pozycje ekranu na punkt swiata na plaszczyznie Y=0.
	 *
	 * Funkcja rzuca promien z kamery przez punkt ekranu i zwraca pozycje
	 * `{x, z}` na plaskiej plaszczyznie pomocniczej.
	 */
	Vector2 screenToWorld(const Camera3D& camera, float screen_x, float screen_y);

} // namespace Nawia::Core
