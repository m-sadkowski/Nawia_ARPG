#include "ProjectileHitEffect.h"
#include <Constants.h>
#include <Logger.h>
#include <cmath>

namespace Nawia::Entity {

    ProjectileHitEffect::ProjectileHitEffect(float x, float y, const std::shared_ptr<Texture2D>& tex)
        : AbilityEffect("ProjectileHit", x, y, tex, AbilityStats()), 
          _current_frame(0), _frame_timer(0.0f), _total_frames(5), _frame_duration(0.15f)
    {
        // Lifetime determined by animation duration
        _stats.duration = _total_frames * _frame_duration;
        _timer = 0.0f;
        
        if (_texture) {
            _frame_width = _texture->width / _total_frames;
            _frame_height = _texture->height;
        } else {
            _frame_width = 0;
            _frame_height = 0;
        }

        // Center the effect visually?
        // Usually explosions should be centered on the hit point
        // _pos is already set to x, y
    }

    void ProjectileHitEffect::update(float dt) {
        AbilityEffect::update(dt); // Handles lifetime decrement

        _frame_timer += dt;
        if (_frame_timer >= _frame_duration) {
            _frame_timer -= _frame_duration;
            _current_frame++;
            if (_current_frame >= _total_frames) {
                // Should be expired by now via AbilityEffect::update, but ensure we don't go out of bounds
                _current_frame = _total_frames - 1;
            }
        }
    }

    void ProjectileHitEffect::render(float camera_x, float camera_y) {
        if (!_texture) return;

        Vector2 screen_pos = getScreenPos(_pos.x, _pos.y, camera_x, camera_y);

        // Define source rectangle for sprite sheet
        Rectangle source = {
            static_cast<float>(_current_frame * _frame_width),
            0.0f,
            static_cast<float>(_frame_width),
            static_cast<float>(_frame_height)
        };

        // Destination rectangle
        // Assuming we want to draw it centered on the screen_pos
        // Adjust scale if needed.
        float dest_width = static_cast<float>(_frame_width); // or Core::TILE_WIDTH?
        float dest_height = static_cast<float>(_frame_height);

        // If texture is small/large, might need scaling. 
        // Let's assume 1:1 for now or scale to tile size.
        // If the texture is a strip of 5 frames, and total width is say 5*64=320, then frame is 64.
        
        Rectangle dest = {
            screen_pos.x - dest_width / 2.0f, // Center horizontally
            screen_pos.y - dest_height / 2.0f, // Center vertically (or at feet?) 
                                               // Projectile hits 'center' now, so center is good.
            dest_width,
            dest_height
        };

        Vector2 origin = { 0.0f, 0.0f }; // Drawing from top-left of dest rect, which is already centered

        DrawTexturePro(*_texture, source, dest, origin, 0.0f, WHITE);
    }

} // namespace Nawia::Entity
