#include "LoadingScreen.h"

#include <Constants.h>
#include <GlobalScaling.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace Nawia::UI {

	// ─── Losowe napisy ───
	static const char* k_loading_quotes[] = {
		"Prosze Panstwa, utopiec bedzie skakal!",
		"Wojownik przychodzi do zielarza i mowi:\nzjadlem dzika i tak brzuch boli, brzuch boli od dzika",
		"-Ojcze, jest kwas chlebowy?  -To se wez...",
		"Sortowanie ekwipunku: zloto, lupy i borowe duchy.",
		"Specjalny miecz na upiory?\nA komu to potrzebne? A dlaczego?",
		"Napelnianie rogow piwem...",
		"Wybudzanie Leszego z zimowego snu...",
		"Ostrzenie widel na spotkanie ze Strzyga...",
		"Pamietaj, ze kazda Rusalka to 10/10...\ndopoki nie wciagnie cie pod wode.",
		"Nie jedz zoltego sniegu.\nZwlaszcza jesli w poblizu kreci sie Czart.",
		"-Nie wlazles! -Jestem herosem",
		"-Ktos nam podpalil chalupe!\n-Co podpalil?\n-Nie wiem kto.\n-Ale co podpalil?\n-No nie wiem kto.",
		"Bogowie, czy Wy to widzicie?",
		"-Z czego ten miecz?  -Amelinium.",
		"Wypelnianie lasu: losie, jelenie, sarny, dziki,\nlisy, borsuki, kuny, jenoty, wilki i rysie.",
	};
	static constexpr int k_quote_count = sizeof(k_loading_quotes) / sizeof(k_loading_quotes[0]);
	static constexpr float k_quote_interval = 4.0f; // sekundy miedzy zmianami

	// ─── Statyczne skladowe ───
	Texture2D LoadingScreen::_background = {};
	Font LoadingScreen::_font = {};
	bool LoadingScreen::_loaded = false;
	int LoadingScreen::_quote_index = 0;
	float LoadingScreen::_quote_timer = 0.0f;

	void LoadingScreen::ensureResources() {
		if (_loaded) return;

		_background = LoadTexture("assets/textures/loading_screen.png");
		if (_background.id > 0)
			SetTextureFilter(_background, TEXTURE_FILTER_BILINEAR);

		_font = LoadFontEx("assets/fonts/slavic_font.ttf", Core::GlobalScaling::scaledInt(300), nullptr, 0);
		if (_font.texture.id > 0) {
			GenTextureMipmaps(&_font.texture);
			SetTextureFilter(_font.texture, TEXTURE_FILTER_TRILINEAR);
		}

		_quote_index = GetRandomValue(0, k_quote_count - 1);
		_quote_timer = 0.0f;
		_loaded = true;
	}

	void LoadingScreen::unload() {
		if (!_loaded) return;

		if (_background.id > 0) UnloadTexture(_background);
		if (_font.texture.id > 0) UnloadFont(_font);

		_background = {};
		_font = {};
		_loaded = false;
		_quote_index = 0;
		_quote_timer = 0.0f;
	}

	void LoadingScreen::render(const float progress, const std::string& /*status_label*/, const std::string& /*title*/) {
		ensureResources();

		const float screen_w = static_cast<float>(GetScreenWidth());
		const float screen_h = static_cast<float>(GetScreenHeight());
		const float clamped = std::clamp(progress, 0.0f, 1.0f);
		const bool has_font = _font.texture.id > 0;

		// ─── Tło: statyczne, cover-fit ───
		if (_background.id > 0) {
			float src_w = static_cast<float>(_background.width);
			float src_h = static_cast<float>(_background.height);
			const float screen_ar = screen_w / screen_h;
			const float tex_ar = src_w / src_h;

			if (tex_ar > screen_ar) src_w = src_h * screen_ar;
			else                    src_h = src_w / screen_ar;

			const Rectangle src_rect = {
				(_background.width - src_w) * 0.5f,
				(_background.height - src_h) * 0.5f,
				src_w, src_h
			};
			DrawTexturePro(_background, src_rect, {0, 0, screen_w, screen_h}, {0, 0}, 0.0f, WHITE);
		} else {
			DrawRectangle(0, 0, static_cast<int>(screen_w), static_cast<int>(screen_h), Color{18, 16, 22, 255});
		}

		// Lekkie przyciemnienie zeby panel byl czytelny.
		DrawRectangle(0, 0, static_cast<int>(screen_w), static_cast<int>(screen_h), Color{0, 0, 0, 140});

		// ─── Napis "Wczytywanie" nad paskiem ───
		const char* header = "Wczytywanie";
		const float header_size = screen_h * 0.06f;
		const float spacing = screen_h * 0.002f;

		if (has_font) {
			const Vector2 hsz = MeasureTextEx(_font, header, header_size, spacing);
			const Vector2 hpos = {(screen_w - hsz.x) * 0.5f, screen_h * 0.5f - screen_h * 0.09f};
			drawTextWithShadow(_font, header, hpos, header_size, spacing, COLOR_PARCHMENT, {3.0f, 3.0f}, Fade(BLACK, 0.6f));
		} else {
			const int sz = static_cast<int>(screen_h * 0.055f);
			const int tw = MeasureText(header, sz);
			DrawText(header, static_cast<int>((screen_w - tw) * 0.5f),
				static_cast<int>(screen_h * 0.5f - screen_h * 0.09f), sz, COLOR_PARCHMENT);
		}

		// ─── Pasek postępu (wycentrowany) ───
		const float bar_w = screen_w * 0.5f;
		const float bar_h = screen_h * 0.035f;
		const float bar_x = (screen_w - bar_w) * 0.5f;
		const float bar_y = screen_h * 0.5f - bar_h * 0.5f;
		const float border_w = screen_h * 0.003f;

		// Tlo paska.
		DrawRectangleRounded({bar_x, bar_y, bar_w, bar_h}, 0.5f, 10, Color{35, 30, 40, 220});

		// Wypelnienie.
		if (clamped > 0.0f) {
			DrawRectangleRounded({bar_x, bar_y, bar_w * clamped, bar_h}, 0.5f, 10, Color{186, 130, 52, 255});
			DrawRectangleRounded({bar_x, bar_y, bar_w * clamped, bar_h * 0.4f}, 0.5f, 10, Fade(WHITE, 0.08f));
		}

		// Obramowka.
		DrawRectangleRoundedLinesEx({bar_x, bar_y, bar_w, bar_h}, 0.5f, 10, border_w, Fade(GOLD, 0.4f));

		// Procent (w pasku).
		const int pct = static_cast<int>(clamped * 100.0f + 0.5f);
		const std::string pct_text = std::to_string(pct) + "%";
		const float pct_size = screen_h * 0.028f;

		if (has_font) {
			const Vector2 psz = MeasureTextEx(_font, pct_text.c_str(), pct_size, spacing);
			const Vector2 ppos = {(screen_w - psz.x) * 0.5f, bar_y + (bar_h - psz.y) * 0.5f};
			drawTextWithShadow(_font, pct_text.c_str(), ppos, pct_size, spacing, WHITE, {1.0f, 1.0f}, Fade(BLACK, 0.5f));
		} else {
			const int sz = static_cast<int>(pct_size);
			const int tw = MeasureText(pct_text.c_str(), sz);
			DrawText(pct_text.c_str(), static_cast<int>((screen_w - tw) * 0.5f),
				static_cast<int>(bar_y + (bar_h - sz) * 0.5f), sz, WHITE);
		}

		// ─── Losowy napis pod paskiem ───
		_quote_timer += GetFrameTime();
		if (_quote_timer >= k_quote_interval) {
			_quote_timer = 0.0f;
			int new_index = GetRandomValue(0, k_quote_count - 2);
			if (new_index >= _quote_index) new_index++;  // Unikamy powtorki.
			_quote_index = new_index;
		}

		const char* quote = k_loading_quotes[_quote_index];
		const float quote_size = screen_h * 0.032f;
		const float quote_y = bar_y + bar_h + screen_h * 0.04f;

		if (has_font) {
			const Vector2 qsz = MeasureTextEx(_font, quote, quote_size, spacing);
			const Vector2 qpos = {(screen_w - qsz.x) * 0.5f, quote_y};
			drawTextWithShadow(_font, quote, qpos, quote_size, spacing, Fade(COLOR_PARCHMENT, 0.65f), {1.0f, 1.0f}, Fade(BLACK, 0.4f));
		} else {
			const int sz = static_cast<int>(quote_size);
			const int tw = MeasureText(quote, sz);
			DrawText(quote, static_cast<int>((screen_w - tw) * 0.5f), static_cast<int>(quote_y), sz,
				Fade(COLOR_PARCHMENT, 0.65f));
		}
	}

} // namespace Nawia::UI
