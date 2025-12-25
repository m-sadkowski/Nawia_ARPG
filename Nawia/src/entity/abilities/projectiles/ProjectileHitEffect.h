#pragma once

#include "AbilityEffect.h"
#include <memory>
#include <raylib.h>

namespace Nawia::Entity {

    class ProjectileHitEffect : public AbilityEffect {
    public:
        ProjectileHitEffect(float x, float y, const std::shared_ptr<Texture2D>& tex);

        void update(float dt) override;
        void render(float offset_x, float offset_y) override;

        // No collision needed for visual effect
        [[nodiscard]] bool checkCollision(const std::shared_ptr<Entity>& target) const override { return false; }
        
    private:
        int _frame_width;
        int _frame_height;
        int _current_frame;
        float _frame_timer;
        int _total_frames;
        float _frame_duration;
    };

} // namespace Nawia::Entity
