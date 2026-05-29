#pragma once

#include <Constants.h>

#include <raylib.h>

namespace Nawia::Entity { class Entity; }

namespace Nawia::Core {

	/**
	 * @class GameCamera
	 * @brief Opakowuje Camera3D i prowadzi ja za wskazana encja.
	 *
	 * Kamera nie posiada celu. `follow()` przyjmuje surowy wskaznik jako
	 * nieposiadajacy widok encji, ktora zyje w EntityManagerze.
	 */
	class GameCamera {
	public:
		GameCamera();

		/**
		 * @brief Obsluguje zoom kamery z kolka myszy.
		 */
		void handleInput();

		/**
		 * @brief Przywraca domyslny zoom uzywany poza trybem developerskim.
		 */
		void resetZoom();
		void resetZoom(float zoom_factor);
		[[nodiscard]] float getZoomFactor() const { return _zoom_factor; }

		/**
		 * @brief Ustawia kamere nad wskazanym celem.
		 */
		void follow(const Entity::Entity* target);

		/**
		 * @brief Zwraca kamere Raylib tylko do odczytu.
		 */
		[[nodiscard]] const Camera3D& get() const;

		/**
		 * @brief Zwraca modyfikowalna kamere Raylib.
		 */
		[[nodiscard]] Camera3D& get();

	private:
		Camera3D _camera_3d = {};
		float _zoom_factor = 1.0f;
	};

} // namespace Nawia::Core
