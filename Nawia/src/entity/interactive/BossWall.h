#pragma once
#include <Entity.h>

namespace Nawia::Entity {

    class BossWall : public Entity {
    public:
        BossWall(float x, float y, float width, float height, Color color = { 180, 40, 40, 120 });

        void update(float dt) override;
        void render(const Camera3D& camera) override;
        
        void takeDamage(int dmg) override { (void)dmg; }

    private:
        Color _wall_color;
        float _pulse_timer = 0.0f;
        float _wall_width;
        float _wall_height;
    };

} // namespace Nawia::Entity
