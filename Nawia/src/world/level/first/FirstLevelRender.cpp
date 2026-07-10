#include "FirstLevel.h"

#include "FirstLevelInternal.h"

#include <Engine.h>
#include <GlobalScaling.h>
#include <UIDefines.h>
#include <UIHandler.h>
#include <UIRenderUtils.h>

#include <algorithm>
#include <cmath>
#include <sstream>

namespace Nawia::World {

	namespace F = FirstLevelSupport;

	namespace {

		void drawIntroParticlesFx(const float width, const float height, const float time) {
			for (int i = 0; i < UI::SMOKE_LAYER_COUNT; ++i) {
				const float seed = static_cast<float>(i) * 11.73f + 3.1f;
				const float travel = UI::fract(UI::hash01(seed) + time * (0.012f + UI::hash01(seed + 2.0f) * 0.016f));
				const float pos_x = width * (0.05f + UI::hash01(seed + 1.0f) * 0.90f) + std::sin(time * (0.22f + UI::hash01(seed + 4.0f) * 0.18f) + seed) * width * 0.06f;
				const float pos_y = height * (1.12f - travel * 1.24f);
				const float radius = Core::GlobalScaling::scaled(110.0f + UI::hash01(seed + 5.0f) * 150.0f);
				const float alpha = (0.35f + (1.0f - travel) * 0.65f) * (0.035f + UI::hash01(seed + 6.0f) * 0.07f);
				DrawCircleGradient(static_cast<int>(pos_x), static_cast<int>(pos_y), radius, UI::withAlpha(LIGHTGRAY, alpha), UI::withAlpha(DARKGRAY, 0.0f));
			}

			for (int i = 0; i < UI::FIRE_PARTICLE_COUNT; ++i) {
				const float seed = static_cast<float>(i) * 17.13f + 8.0f;
				const float cycle = UI::fract(UI::hash01(seed) + time * (0.10f + UI::hash01(seed + 1.0f) * 0.22f));
				const float rise = 1.0f - cycle;
				const float pos_x = width * (0.03f + UI::hash01(seed + 2.0f) * 0.94f) + std::sin(time * (1.0f + UI::hash01(seed + 3.0f) * 1.5f) + seed) * width * (0.01f + UI::hash01(seed + 9.0f) * 0.02f);
				const float pos_y = height * (1.04f - rise * 1.18f);
				const float radius = Core::GlobalScaling::scaled(1.5f + UI::hash01(seed + 7.0f) * UI::hash01(seed + 7.0f) * 12.0f) * (0.45f + rise * 0.95f);
				const float alpha = (0.10f + rise * 0.50f) * (0.55f + UI::hash01(seed + 6.0f) * 0.45f);
				DrawCircleGradient(static_cast<int>(pos_x), static_cast<int>(pos_y), radius, UI::withAlpha(UI::COLOR_GOLDEN_TEXT, alpha), UI::withAlpha(UI::COLOR_SLAVIC_ORANGE, alpha * 0.35f));
			}
		}

		void drawNawiaFogFx(const float width, const float height, const float time) {
			DrawRectangleGradientV(0, 0, static_cast<int>(width), static_cast<int>(height), Color{4, 5, 8, 185}, Color{0, 0, 0, 215});

			for (int i = 0; i < 22; ++i) {
				const float seed = static_cast<float>(i) * 19.41f + 5.7f;
				const float drift = UI::fract(UI::hash01(seed) + time * (0.010f + UI::hash01(seed + 1.0f) * 0.014f));
				const float pos_x = width * (UI::hash01(seed + 2.0f) * 1.15f - 0.08f) + std::sin(time * 0.09f + seed) * width * 0.07f;
				const float pos_y = height * (0.10f + UI::hash01(seed + 3.0f) * 0.92f) + (drift - 0.5f) * height * 0.18f;
				const float radius = Core::GlobalScaling::scaled(95.0f + UI::hash01(seed + 4.0f) * 190.0f);
				const float alpha = 0.035f + UI::hash01(seed + 5.0f) * 0.07f;
				DrawCircleGradient(
					static_cast<int>(pos_x),
					static_cast<int>(pos_y),
					radius,
					UI::withAlpha(Color{115, 124, 138, 255}, alpha),
					UI::withAlpha(BLACK, 0.0f));
			}
		}

		void drawAnimatedIntroImage(const std::shared_ptr<Texture2D>& texture, const float screen_width, const float screen_height, const float time) {
			if (!texture || texture->id <= 0) {
				DrawRectangle(0, 0, static_cast<int>(screen_width), static_cast<int>(screen_height), BLACK);
				return;
			}

			float source_width = static_cast<float>(texture->width);
			float source_height = static_cast<float>(texture->height);
			const float screen_aspect_ratio = screen_width / screen_height;
			const float texture_aspect_ratio = source_width / source_height;

			if (texture_aspect_ratio > screen_aspect_ratio)
				source_width = source_height * screen_aspect_ratio;
			else
				source_height = source_width / screen_aspect_ratio;

			const float zoom_factor = 0.10f + 0.018f * std::sin(time * 0.22f);
			source_width *= (1.0f - zoom_factor);
			source_height *= (1.0f - zoom_factor);

			const float offset_x = std::max(0.0f, (static_cast<float>(texture->width) - source_width) * 0.5f) * std::sin(time * 0.11f + 0.8f);
			const float offset_y = std::max(0.0f, (static_cast<float>(texture->height) - source_height) * 0.5f) * std::cos(time * 0.08f - 0.35f);
			const Rectangle source_rectangle = {
				(static_cast<float>(texture->width) - source_width) * 0.5f + offset_x,
				(static_cast<float>(texture->height) - source_height) * 0.5f + offset_y,
				source_width,
				source_height
			};

			DrawTexturePro(*texture, source_rectangle, {0.0f, 0.0f, screen_width, screen_height}, {0.0f, 0.0f}, 0.0f, WHITE);
		}

	}

	void FirstLevel::renderOverlay(Core::Engine* engine) const {
		const bool render_nawia_fog =
			_current_location_index < _location_definitions.size() &&
			F::isPrzedsionekNawiiLocation(_location_definitions[_current_location_index]);

		if (_intro_phase == IntroPhase::Inactive && _intro_flash_timer <= 0.0f && !render_nawia_fog)
			return;

		const int width = GetScreenWidth();
		const int height = GetScreenHeight();
		const float screen_width = static_cast<float>(width);
		const float screen_height = static_cast<float>(height);
		const float overlay_time = static_cast<float>(GetTime());
		if (render_nawia_fog)
			drawNawiaFogFx(screen_width, screen_height, overlay_time);

		const float alpha = std::clamp(_intro_overlay_alpha, 0.0f, 1.0f);
		const bool rendering_slide =
			(_intro_phase == IntroPhase::Slides || _intro_phase == IntroPhase::OutroSlides) &&
			_intro_slide_index < _intro_slides.size();
		if (!rendering_slide && alpha > 0.0f)
			DrawRectangle(0, 0, width, height, Fade(BLACK, alpha));

		if (rendering_slide && engine) {
			const auto& slide = _intro_slides[_intro_slide_index];
			const float fade_in = std::clamp(_intro_timer / 1.2f, 0.0f, 1.0f);
			const float fade_out = std::clamp((slide.duration - _intro_timer) / 1.2f, 0.0f, 1.0f);
			const float text_alpha = std::min(fade_in, fade_out);
			const float image_time = static_cast<float>(GetTime()) + static_cast<float>(_intro_slide_index) * 13.0f;

			drawAnimatedIntroImage(slide.image_texture, screen_width, screen_height, image_time);
			DrawRectangleGradientV(0, 0, width, height, UI::withAlpha({30, 14, 10, 255}, 0.10f), UI::withAlpha({5, 5, 8, 255}, 0.55f));
			drawIntroParticlesFx(screen_width, screen_height, image_time);
			DrawRectangleGradientV(0, 0, width, height, UI::withAlpha(UI::COLOR_ACCENT, 0.02f), UI::withAlpha(BLACK, 0.18f));
			DrawRectangle(0, 0, width, height, Fade(BLACK, 0.22f + (1.0f - text_alpha) * 0.78f));

			const Font& font = engine->getUIHandler().getFont();
			const float font_size = Core::GlobalScaling::scaled(30.0f);
			const float spacing = Core::GlobalScaling::scaled(1.0f);
			const float max_width = screen_width * 0.74f;

			std::vector<std::string> lines;
			std::istringstream paragraphs(slide.text);
			std::string paragraph;
			while (std::getline(paragraphs, paragraph)) {
				if (paragraph.empty()) {
					lines.emplace_back();
					continue;
				}
				std::istringstream words(paragraph);
				std::string word;
				std::string current_line;
				while (words >> word) {
					const std::string candidate = current_line.empty() ? word : current_line + " " + word;
					if (MeasureTextEx(font, candidate.c_str(), font_size, spacing).x <= max_width || current_line.empty()) {
						current_line = candidate;
					} else {
						lines.push_back(current_line);
						current_line = word;
					}
				}
				if (!current_line.empty())
					lines.push_back(current_line);
			}

			const float line_height = font_size * 1.42f;
			const float text_block_height = static_cast<float>(lines.size()) * line_height;
			float text_y = screen_height * 0.66f - text_block_height * 0.5f;
			for (const auto& line : lines) {
				if (line.empty()) {
					text_y += line_height * 0.58f;
					continue;
				}
				const Vector2 size = MeasureTextEx(font, line.c_str(), font_size, spacing);
				const Vector2 pos = {screen_width * 0.5f - size.x * 0.5f, text_y};
				DrawTextEx(font, line.c_str(), {pos.x + 2.0f, pos.y + 2.0f}, font_size, spacing, UI::withAlpha(BLACK, text_alpha * 0.70f));
				DrawTextEx(font, line.c_str(), pos, font_size, spacing, UI::withAlpha(RAYWHITE, text_alpha));
				text_y += line_height;
			}
		}

		if ((_intro_phase == IntroPhase::Slides || _intro_phase == IntroPhase::FadeFromBlackAfterSlides || _intro_phase == IntroPhase::OutroSlides) && engine)
			F::drawIntroSkipButton(engine->getUIHandler().getFont(), F::getIntroSkipButtonRect(width, height));

		if (_intro_flash_timer > 0.0f) {
			const float flash_alpha = std::clamp(_intro_flash_timer / 0.55f, 0.0f, 1.0f);
			DrawRectangle(0, 0, width, height, Fade(WHITE, flash_alpha * 0.72f));
		}
	}

} // namespace Nawia::World
