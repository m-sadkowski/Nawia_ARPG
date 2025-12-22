
#include "PlayerController.h"
#include "Engine.h"
#include "Logger.h"

#include <EnemyInterface.h>
#include <string>

namespace Nawia::Core {

PlayerController::PlayerController(Engine *engine,
                                   std::shared_ptr<Entity::Player> player)
    : _engine(engine), _player(std::move(player)) {}

void PlayerController::handleInput(const float mouse_world_x,
                                   const float mouse_world_y,
                                   const float screen_x, const float screen_y) {
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    if (const auto entity = _engine->getEntityAt(screen_x, screen_y)) {
      if (const auto enemy =
              std::dynamic_pointer_cast<Entity::EnemyInterface>(entity)) {
        _target_enemy = enemy;
        return;
      }
    }

    _target_enemy = nullptr;
    _player->moveTo(mouse_world_x, mouse_world_y);
  }

  int ability_index = -1;

  if (IsKeyPressed(KEY_Q))
    ability_index = 0;
  if (IsKeyPressed(KEY_W))
    ability_index = 1;
  if (IsKeyPressed(KEY_E))
    ability_index = 2;
  if (IsKeyPressed(KEY_R))
    ability_index = 3;

  if (const auto ability = _player->getAbility(ability_index)) {
    switch (ability->getTargetType()) {
    case Entity::AbilityTargetType::UNIT:
      if (const auto target = _engine->getEntityAt(screen_x, screen_y))
        if (const auto enemy =
                std::dynamic_pointer_cast<Entity::EnemyInterface>(target))
          useAbility(ability_index, enemy->getX(), enemy->getY());
      break;
    case Entity::AbilityTargetType::POINT:
      useAbility(ability_index, mouse_world_x, mouse_world_y);
      break;
    case Entity::AbilityTargetType::SELF:
    default:
      break;
    }
  }
}

void PlayerController::update(float dt) {
  if (!_target_enemy)
    return;

  if (_target_enemy->isDead()) {
    _target_enemy = nullptr;
    return;
  }

  const float dx = _target_enemy->getX() - _player->getX();
  const float dy = _target_enemy->getY() - _player->getY();
  const float dist_sq = dx * dx + dy * dy;

  constexpr int auto_attack_index = 0;
  const float attack_cast_range =
      _player->getAbility(auto_attack_index)->getCastRange();

  // Hysteresis buffer to prevent flickering
  // We stop moving if we are within range, but we only START moving again if we
  // are a bit further away This "dead zone" prevents rapid state switching
  const float hysteresis = 0.5f;

  if (dist_sq >
      (attack_cast_range + hysteresis) * (attack_cast_range + hysteresis)) {
    _player->moveTo(_target_enemy->getX(), _target_enemy->getY());
  } else if (dist_sq <= attack_cast_range * attack_cast_range) {
    // we stop the player by moving him to his current location
    // But only if we are properly inside the range
    _player->moveTo(_player->getX(), _player->getY());
    useAbility(auto_attack_index, _target_enemy->getX(), _target_enemy->getY());
  }
  // checking boundaries [range, range + hysteresis] - do nothing (keep previous
  // state) except we need to ensure we attack if we are close enough? actually
  // if we are simply "standing still" in that zone, we should attack. So the
  // else block covers being in range. The first block covers being OUT of range
  // + buffer. What if we are in the buffer zone? We probably want to keep doing
  // what we were doing. If we were moving, we keep moving? No, that's complex
  // to track without state.
  //
  // Simpler approach:
  // If (> range), move.
  // If (<= range), stop and attack.
  // The flicker happens because > range becomes <= range next frame, then >
  // range again.
  //
  // Let's just slightly reduce the "stop" distance check or increase the move
  // check. If we are moving, we aim for range - epsilon.
  //
  // Revised approach: Use a standard hysteresis check.

  if (dist_sq > attack_cast_range * attack_cast_range) {
    // We are outside range.
    // Only move if we are SIGNIFICANTLY outside range to avoid
    // micro-adjustments if we just stopped? Or: when moving TO target, aim for
    // a point slightly CLOSER than max range to ensure we stay in range.

    // Let's change the moveTo target? No, checking distance is better.

    const float tolerance = 0.2f; // buffer
    if (dist_sq >
        (attack_cast_range + tolerance) * (attack_cast_range + tolerance)) {
      _player->moveTo(_target_enemy->getX(), _target_enemy->getY());
    }
  } else {
    // We are inside range (or very close). Stop and attack.
    _player->moveTo(_player->getX(), _player->getY());
    useAbility(auto_attack_index, _target_enemy->getX(), _target_enemy->getY());
  }
}

void PlayerController::useAbility(const int index, const float target_x,
                                  const float target_y) const {
  if (const auto spell = _player->getAbility(index)) {
    if (!spell->isReady())
      return;

    if (auto effect = spell->cast(_player.get(), target_x, target_y)) {
      Logger::debugLog("Ability " + std::to_string(index) +
                       " used, target location: (" + std::to_string(target_x) +
                       ", " + std::to_string(target_y) + ")");
      _engine->spawnEntity(std::move(effect));
    }
  }
}

} // namespace Nawia::Core
