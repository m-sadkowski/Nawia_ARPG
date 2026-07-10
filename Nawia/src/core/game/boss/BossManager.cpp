#include "BossManager.h"

#include <BossDefinitionLoader.h>

namespace Nawia::Game {

    BossManager::BossManager() {}
    BossManager::~BossManager() {}

    void BossManager::loadFromJson(const std::string& path) {
        _bosses = BossDefinitionLoader::loadFromJson(path);
    }

    std::vector<std::string> BossManager::getDefeatedBossIds() const {
        return {_defeated_bosses.begin(), _defeated_bosses.end()};
    }

    void BossManager::setDefeatedBossIds(const std::vector<std::string>& boss_ids) {
        _defeated_bosses.clear();
        for (const auto& boss_id : boss_ids) {
            if (!boss_id.empty())
                _defeated_bosses.insert(boss_id);
        }
    }

} // namespace Nawia::Game
