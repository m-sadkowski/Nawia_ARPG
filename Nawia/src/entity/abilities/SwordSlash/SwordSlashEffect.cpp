#include "SwordSlashEffect.h"
#include "Constants.h"
#include <Logger.h>
#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

	SwordSlashEffect::SwordSlashEffect(const float x, const float y, const float angle, const std::shared_ptr<SDL_Texture> &tex, const int damage)
		: AbilityEffect(x, y, tex, 0.2f, damage), _angle(angle) {}

	void SwordSlashEffect::update(const float dt) { AbilityEffect::update(dt); }

	void SwordSlashEffect::render(SDL_Renderer* renderer, const float camera_x, const float camera_y) {
	  if (!_texture) {
	    Core::Logger::errorLog("SwordSlashEffect - Could not load texture.");
	    return;
	  }

	  Core::Point2D screen_pos = getScreenPos(_pos->getX(), _pos->getY(), camera_x, camera_y);
	  const SDL_FRect dest_rect = {screen_pos.getX(), screen_pos.getY(),Core::ENTITY_TEXTURE_WIDTH,Core::ENTITY_TEXTURE_HEIGHT};
	  constexpr SDL_FPoint center = {Core::ENTITY_TEXTURE_WIDTH / 4.0f,Core::ENTITY_TEXTURE_HEIGHT / 4.0f};

	  SDL_RenderTextureRotated(renderer, _texture.get(), nullptr, &dest_rect, _angle + 70.0f, &center, SDL_FLIP_NONE);
	}

	bool SwordSlashEffect::hasHit(const void* entity) const {
	  return std::find(_hit_entities.begin(), _hit_entities.end(), entity) != _hit_entities.end();
	}

	void SwordSlashEffect::addHit(const void* entity) {
	  _hit_entities.push_back(entity);
	}

} // namespace Nawia::Entity
