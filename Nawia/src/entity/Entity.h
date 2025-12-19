#pragma once
#include "MathUtils.h"


#include <SDL3/SDL.h>
#include <iostream>
#include <memory>


namespace Nawia::Entity {

class Entity {
public:
  Entity(float start_x, float start_y,
         const std::shared_ptr<SDL_Texture> &texture, int max_hp);
  virtual ~Entity() = default;

  virtual void update(float delta_time) = 0;
  virtual void render(SDL_Renderer *renderer, float offset_x, float offset_y);

  virtual void takeDamage(int dmg);

  bool isDead() const { return _hp <= 0; }
  int getHP() const { return _hp; }
  int getMaxHP() const { return _max_hp; }
  std::string getName() { return ""; };

  bool isMouseOver(float mouse_x, float mouse_y, float cam_x,
                   float cam_y) const;

  float getX() const { return _pos->getX(); }
  float getY() const { return _pos->getY(); }

  Core::Point2D getScreenPos(float mouse_x, float mouse_y, float cam_x,
                             float cam_y) const;

protected:
  // position in game - uses Point2D
  // while creating an entity, pass (x, y)
  // and it will be converted to a Point2D assigned to said entity
  // to get/set values use _pos->getX/Y or _pos->setX/Y
  std::unique_ptr<Core::Point2D> _pos;
  std::shared_ptr<SDL_Texture> _texture;

  int _hp;
  int _max_hp;
};

} // namespace Nawia::Entity