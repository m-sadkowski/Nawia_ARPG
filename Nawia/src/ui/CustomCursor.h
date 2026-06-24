#pragma once

#include <raylib.h>

#include <memory>
#include <string>

namespace Nawia::Core {
	class ResourceManager;
}

namespace Nawia::UI {

	/**
	 * @enum CursorState
	 * @brief Aktualny tryb kursora okreslajacy jego wyglad.
	 */
	enum class CursorState {
		Default,   ///< Standardowa strzalka-wskaznik.
		Interact   ///< Reka interakcji (hover nad clickable/NPC/chest).
	};

	/**
	 * @class CustomCursor
	 * @brief Rysuje customowy kursor gry w stylu slowanskim zamiast systemowego.
	 *
	 * Kursor laduje dwie tekstury (domyslna strzalka i reka interakcji)
	 * i renderuje ja na pozycji myszy z delikatnym efektem poswiety.
	 * System automatycznie chowa kursor systemowy.
	 */
	class CustomCursor {
	public:
		CustomCursor() = default;

		/** @brief Laduje tekstury kursora i chowa kursor systemowy. */
		void initialize(Core::ResourceManager& resource_manager);

		/** @brief Ustawia aktywny stan kursora. */
		void setState(CursorState state) { _state = state; }

		/** @brief Zwraca aktywny stan kursora. */
		[[nodiscard]] CursorState getState() const { return _state; }

		/**
		 * @brief Renderuje kursor na aktualnej pozycji myszy.
		 *
		 * Wywolywane tuz przed EndDrawing() w petli renderujacej,
		 * zeby kursor byl zawsze na wierzchu.
		 */
		void render() const;

	private:
		std::shared_ptr<Texture2D> _cursor_default;
		std::shared_ptr<Texture2D> _cursor_interact;
		CursorState _state = CursorState::Default;

		/** @brief Rozmiar kursora w pikselach (skalowalny). */
		static constexpr float BASE_CURSOR_SIZE = 40.0f;

		/** @brief Promien poswiety wokol czubka kursora. */
		static constexpr float GLOW_RADIUS = 18.0f;
	};

} // namespace Nawia::UI
