#include "BossPhaseMath.h"

#include <algorithm>
#include <cmath>

namespace Nawia::Game::BossPhaseMath {

	int restartHp(const BossData& boss, const int phase_index, const int max_hp)
	{
		if (max_hp <= 0)
			return 1;

		if (phase_index <= 0 || boss.phases.empty())
			return max_hp;

		const int clamped_phase = std::clamp(
			phase_index,
			0,
			static_cast<int>(boss.phases.size()) - 1);
		const int threshold_hp = static_cast<int>(std::floor(
			static_cast<float>(max_hp) * boss.phases[clamped_phase].hp_threshold));

		return std::clamp(threshold_hp + 1, 1, max_hp);
	}

	int resolvePhaseIndexForHp(
		const BossData& boss,
		const int current_phase_index,
		const int hp,
		const int max_hp)
	{
		if (boss.phases.empty() || max_hp <= 0)
			return std::max(0, current_phase_index);

		int result = std::clamp(
			current_phase_index,
			0,
			static_cast<int>(boss.phases.size()) - 1);
		const float hp_pct = static_cast<float>(hp) / static_cast<float>(max_hp);
		for (int i = static_cast<int>(boss.phases.size()) - 1; i > result; --i) {
			if (hp_pct <= boss.phases[i].hp_threshold) {
				result = i;
				break;
			}
		}

		return result;
	}

} // namespace Nawia::Game::BossPhaseMath
