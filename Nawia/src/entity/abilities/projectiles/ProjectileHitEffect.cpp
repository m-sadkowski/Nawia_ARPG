#include "ProjectileHitEffect.h"

#include <Constants.h>
#include <Logger.h>
#include <cmath>

namespace Nawia::Entity {

    ProjectileHitEffect::ProjectileHitEffect(const float x, const float y, const std::shared_ptr<Texture2D>& tex)
        : AbilityEffect("ProjectileHit", x, y, tex, AbilityStats()), 
          _current_frame(0), _frame_timer(0.0f), _total_frames(1), _frame_duration(0.5f)
    {
        // lifetime determined by animation duration
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
        AbilityEffect::update(dt); // handles lifetime decrement

        _frame_timer += dt;
        if (_frame_timer >= _frame_duration) {
            _frame_timer -= _frame_duration;
            _current_frame++;
            if (_current_frame >= _total_frames) {
                _current_frame = _total_frames - 1;
            }
        }
    }

    void ProjectileHitEffect::render(const float offset_x, const float offset_y) {
        if (!_texture) return;

        const Vector2 screen_pos = getScreenPos(_pos.x, _pos.y, offset_x, offset_y);

        // define source rectangle for sprite sheet
        const Rectangle source = {
            static_cast<float>(_current_frame * _frame_width),
            0.0f,
            static_cast<float>(_frame_width),
            static_cast<float>(_frame_height)
        };

        const float dest_width = static_cast<float>(_frame_width);
        const float dest_height = static_cast<float>(_frame_height);

        const Rectangle dest = {
            screen_pos.x - dest_width / 2.0f, // center horizontally
            screen_pos.y - dest_height / 2.0f, // center vertically
            dest_width,
            dest_height
        };

        constexpr Vector2 origin = { 0.0f, 0.0f }; // drawing from top-left of dest rect, which is already centered

        DrawTexturePro(*_texture, source, dest, origin, 0.0f, WHITE);
    }

} // namespace Nawia::Entity
