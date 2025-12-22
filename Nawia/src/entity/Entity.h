#pragma once
#include <MathUtils.h>

#include <raylib.h>
#include <iostream>
#include <memory>

namespace Nawia::Entity {

	class Entity {
	public:
		Entity(float start_x, float start_y, const std::shared_ptr<Texture2D>& texture, int max_hp);
		virtual ~Entity() = default;

		virtual void update(float delta_time) = 0;
		virtual void render(float offset_x, float offset_y);

		[[nodiscard]] float getX() const { return _pos->getX(); }
		[[nodiscard]] float getY() const { return _pos->getY(); }
		[[nodiscard]] Core::Point2D getScreenPos(float mouse_x, float mouse_y, float cam_x, float cam_y) const;
		[[nodiscard]] virtual bool isMouseOver(float mouse_x, float mouse_y, float cam_x, float cam_y) const;

		virtual void takeDamage(int dmg);
		void die();
		[[nodiscard]] bool isDead() const { return _hp <= 0; }
		[[nodiscard]] int getHP() const { return _hp; }
		[[nodiscard]] int getMaxHP() const { return _max_hp; }
		[[nodiscard]] std::string getName() { return ""; };

	protected:
		std::unique_ptr<Core::Point2D> _pos;
		std::shared_ptr<Texture2D> _texture;

		int _hp;
		int _max_hp;
	};

} // namespace Nawia::Entity