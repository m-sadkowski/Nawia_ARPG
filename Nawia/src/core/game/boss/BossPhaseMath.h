#pragma once

#include <BossTypes.h>

namespace Nawia::Game::BossPhaseMath {

	[[nodiscard]] int restartHp(const BossData& boss, int phase_index, int max_hp);
	[[nodiscard]] int resolvePhaseIndexForHp(const BossData& boss, int current_phase_index, int hp, int max_hp);

} // namespace Nawia::Game::BossPhaseMath
