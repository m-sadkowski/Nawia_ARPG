#pragma once

#include <raylib.h>
#include <memory>
#include <vector>

namespace Nawia::Entity {

    class Entity;

    enum class ColliderType {
        NONE,
        CIRCLE,
        RECTANGLE,
        CONE
    };

    class Collider {
    public:
        Collider(Entity* owner, const float offset_x = 0.0f, const float offset_y = 0.0f) : _owner(owner), _offset(Vector2{offset_x, offset_y}) {}
        virtual ~Collider() = default;

        [[nodiscard]] virtual ColliderType getType() const = 0;
        [[nodiscard]] virtual bool checkCollision(const Collider* other) const = 0;
        [[nodiscard]] virtual bool checkPoint(float screen_x, float screen_y, float cam_x, float cam_y) const = 0;
        virtual void render(const float offset_x, const float offset_y) const = 0;

        void setOffset(const float x, const float y) { _offset = Vector2{ x, y }; }
        [[nodiscard]] Vector2 getPosition() const;

    protected:
        Entity* _owner;
        Vector2 _offset;
    };

    class CircleCollider : public Collider {
    public:
        CircleCollider(Entity* owner, const float radius, const float offset_x = 0.0f, const float offset_y = 0.0f)
            : Collider(owner, offset_x, offset_y), _radius(radius) {}

        [[nodiscard]] ColliderType getType() const override { return ColliderType::CIRCLE; }
        [[nodiscard]] bool checkCollision(const Collider* other) const override;
        [[nodiscard]] bool checkPoint(float screen_x, float screen_y, float cam_x, float cam_y) const override;
        void render(float offset_x, float offset_y) const override;

        [[nodiscard]] float getRadius() const { return _radius; }

    private:
        float _radius;
    };

    class RectangleCollider : public Collider {
    public:
        RectangleCollider(Entity* owner, const float width, const float height, const float offset_x = 0.0f, const float offset_y = 0.0f)
            : Collider(owner, offset_x, offset_y), _width(width), _height(height) {}

        [[nodiscard]] ColliderType getType() const override { return ColliderType::RECTANGLE; }
        [[nodiscard]] bool checkCollision(const Collider* other) const override;
        [[nodiscard]] bool checkPoint(float screen_x, float screen_y, float cam_x, float cam_y) const override;
        void render(float offset_x, float offset_y) const override;

        [[nodiscard]] float getWidth() const { return _width; }
        [[nodiscard]] float getHeight() const { return _height; }
        [[nodiscard]] Rectangle getRect() const;

    private:
        float _width;
        float _height;
    };

    class ConeCollider : public Collider {
    public:
        ConeCollider(Entity* owner, const float radius, const float angle, const float offset_x = 0.0f, const float offset_y = 0.0f)
            : Collider(owner, offset_x, offset_y), _radius(radius), _angle(angle) {}

        [[nodiscard]] ColliderType getType() const override { return ColliderType::CONE; }
        [[nodiscard]] bool checkCollision(const Collider* other) const override;
        [[nodiscard]] bool checkPoint(float screen_x, float screen_y, float cam_x, float cam_y) const override;
        void render(float offset_x, float offset_y) const override;


        [[nodiscard]] float getRadius() const { return _radius; }
        [[nodiscard]] float getAngle() const { return _angle; } 

    private:
        float _radius;
        float _angle;
    };

} // namespace Nawia::Entity
