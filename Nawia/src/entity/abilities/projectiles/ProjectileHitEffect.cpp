#include "ProjectileHitEffect.h"

namespace Nawia::Entity {

	ProjectileHitEffect::ProjectileHitEffect(const float x, const float y, const std::shared_ptr<Texture2D>& tex)
		: AbilityEffect("ProjectileHit", x, y, tex, AbilityStats()) {
		_stats.duration = _total_frames * _frame_duration;

		if (const auto& texture = getTexture()) {
			_frame_width = texture->width / _total_frames;
			_frame_height = texture->height;
		}
	}

	void ProjectileHitEffect::update(const float dt) {
		AbilityEffect::update(dt);

		_frame_timer += dt;
		if (_frame_timer < _frame_duration)
			return;

		_frame_timer -= _frame_duration;
		if (_current_frame < _total_frames - 1)
			++_current_frame;
	}

	void ProjectileHitEffect::render([[maybe_unused]] const Camera3D& camera) {
		// DO ZROBIENIA: renderować trafienie jako płaski sprite albo cząsteczkę 3D.
		// Na razie to efekt wizualny, który sam wygasa po czasie.
	}

} // namespace Nawia::Entity
