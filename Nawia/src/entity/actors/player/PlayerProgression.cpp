#include "Player.h"

#include <SoundIds.h>

#include <algorithm>
#include <cmath>

namespace Nawia::Entity {

	void Player::levelUp() {
		_level++;
		_exp = _exp - _exp_to_next_lvl;
		_exp_to_next_lvl = _exp_to_next_lvl + 1000;
		_base_stats.max_hp = _base_stats.max_hp + 15;
		_base_stats.damage = _base_stats.damage + 2;
		_base_stats.attack_speed = _base_stats.attack_speed + 0.1f;
		_base_stats.movement_speed = 4.0f;
		recalculateStats();
	}

	void Player::isLevelUp() {
		if (_exp >= _exp_to_next_lvl)
			levelUp();
	}

	void Player::takeDamage(const int dmg) {
		const bool was_dying = isDying();
		const float damage_reduction = std::clamp(static_cast<float>(_current_stats.defense) * 0.005f, 0.0f, 0.9f);
		const int reduced_damage = dmg > 0
			? std::max(1, static_cast<int>(std::round(static_cast<float>(dmg) * (1.0f - damage_reduction))))
			: dmg;

		Entity::takeDamage(reduced_damage);
		if (!isDying())
			playSoundEffect(Audio::SoundId::PlayerHurt, 0.85f);

		if (!was_dying && isDying()) {
			stop();
			setAnimationSpeed(2.0f);
		}
	}

} // namespace Nawia::Entity
