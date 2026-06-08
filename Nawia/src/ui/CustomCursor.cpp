#include "CustomCursor.h"

#include <GlobalScaling.h>
#include <ResourceManager.h>

#include <cmath>

namespace Nawia::UI {

	void CustomCursor::initialize(Core::ResourceManager& resource_manager) {
		_cursor_default = resource_manager.getTexture("assets/textures/ui/cursor_default.png");
		_cursor_interact = resource_manager.getTexture("assets/textures/ui/cursor_interact.png");

		if (_cursor_default) {
			GenTextureMipmaps(_cursor_default.get());
			SetTextureFilter(*_cursor_default, TEXTURE_FILTER_TRILINEAR);
		}
		if (_cursor_interact) {
			GenTextureMipmaps(_cursor_interact.get());
			SetTextureFilter(*_cursor_interact, TEXTURE_FILTER_TRILINEAR);
		}

		HideCursor();
	}

	void CustomCursor::render() const {
		const Vector2 mouse_pos = GetMousePosition();
		const float cursor_size = Core::GlobalScaling::scaled(BASE_CURSOR_SIZE);
		const float glow_radius = Core::GlobalScaling::scaled(GLOW_RADIUS);
		const float time = static_cast<float>(GetTime());

		// Pulsujaca poswieta na czubku kursora.
		const float glow_alpha = 0.12f + 0.06f * std::sin(time * 3.0f);
		const Color glow_color = (_state == CursorState::Interact)
			? Color{255, 200, 100, static_cast<unsigned char>(glow_alpha * 255.0f)}
			: Color{255, 180, 80, static_cast<unsigned char>(glow_alpha * 0.7f * 255.0f)};

		DrawCircleGradient(
			static_cast<int>(mouse_pos.x),
			static_cast<int>(mouse_pos.y),
			glow_radius,
			glow_color,
			Color{0, 0, 0, 0}
		);

		// Wybieramy aktywna teksture kursora.
		const Texture2D* active_texture = nullptr;
		if (_state == CursorState::Interact && _cursor_interact && _cursor_interact->id > 0)
			active_texture = _cursor_interact.get();
		else if (_cursor_default && _cursor_default->id > 0)
			active_texture = _cursor_default.get();

		if (active_texture) {
			const Rectangle source = {
				0.0f, 0.0f,
				static_cast<float>(active_texture->width),
				static_cast<float>(active_texture->height)
			};

			// Hotspot kursora w lewym gornym rogu (czubek strzalki).
			const Rectangle dest = {
				mouse_pos.x,
				mouse_pos.y,
				cursor_size,
				cursor_size
			};

			// Lekki cien pod kursorem dla czytelnosci.
			const Rectangle shadow_dest = {
				mouse_pos.x + Core::GlobalScaling::scaled(2.0f),
				mouse_pos.y + Core::GlobalScaling::scaled(2.0f),
				cursor_size,
				cursor_size
			};
			DrawTexturePro(*active_texture, source, shadow_dest, {0.0f, 0.0f}, 0.0f, Color{0, 0, 0, 80});
			DrawTexturePro(*active_texture, source, dest, {0.0f, 0.0f}, 0.0f, WHITE);
		}
	}

} // namespace Nawia::UI
