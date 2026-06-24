#pragma once

#include <raylib.h>

#include <string>

namespace Nawia::UI {

	/**
	 * @class LoadingScreen
	 * @brief Ekran ladowania z paskiem postepu i losowymi napisami.
	 */
	class LoadingScreen {
	public:
		static void render(float progress, const std::string& status_label, const std::string& title);
		static void unload();

	private:
		static void ensureResources();

		static Texture2D _background;
		static Font _font;
		static bool _loaded;
		static int _quote_index;
		static float _quote_timer;
	};

} // namespace Nawia::UI
