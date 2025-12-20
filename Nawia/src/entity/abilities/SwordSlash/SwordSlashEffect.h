#pragma once
#include <AbilityEffect.h>
#include <vector>

namespace Nawia::Entity {

class SwordSlashEffect : public AbilityEffect {
public:
  SwordSlashEffect(float x, float y, float angle, const std::shared_ptr<SDL_Texture> &tex, int damage);

  void update(float dt) override;
  void render(SDL_Renderer *renderer, float camera_x, float camera_y) override;

  float getAngle() const { return _angle; }
  bool hasHit(const void* entity) const;
  void addHit(const void* entity);

private:
  float _angle;
  std::vector<const void*> _hit_entities;
};

} // namespace Nawia::Entity
