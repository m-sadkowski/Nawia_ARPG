#include "ProjectileHitEffect.h"

#include <Logger.h>
#include <cmath>

namespace Nawia::Entity {

    ProjectileHitEffect::ProjectileHitEffect(const float x, const float y, const std::shared_ptr<Texture2D>& tex)
        : AbilityEffect("ProjectileHit", x, y, tex, AbilityStats()), 
          _current_frame(0), _frame_timer(0.0f), _total_frames(1), _frame_duration(0.5f)
    {
        _stats.duration = _total_frames * _frame_duration;
        _timer = 0.0f;
        
        if (_texture) {
            _frame_width = _texture->width / _total_frames;
            _frame_height = _texture->height;
        } else {
            _frame_width = 0;
            _frame_height = 0;
        }
    }

    void ProjectileHitEffect::update(const float dt) {
        AbilityEffect::update(dt);

        _frame_timer += dt;
        if (_frame_timer >= _frame_duration) {
            _frame_timer -= _frame_duration;
            _current_frame++;
            if (_current_frame >= _total_frames) {
                _current_frame = _total_frames - 1;
            }
        }
    }

    void ProjectileHitEffect::render(const Camera3D& camera) {
		// DO ZROBIENIA: renderować trafienie jako płaski sprite albo cząsteczkę 3D.
        // Na razie to efekt wizualny, który sam wygasa po czasie.
    }

} // namespace Nawia::Entity
