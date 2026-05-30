#include "BossManager.h"

#include <ActorInterface.h>
#include <Bandit.h>
#include <Collider.h>
#include <Devil.h>
#include <Engine.h>
#include <Frog.h>
#include <Logger.h>
#include <Map.h>
#include <WalkingDead.h>
#include <json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>

namespace Nawia::Game {

    BossManager::BossManager() {}
    BossManager::~BossManager() {}

    // -----------------------------------------------------------------------
    // Helpery parsowania JSON (lokalne w jednostce kompilacji)
    // -----------------------------------------------------------------------

    namespace {

        std::vector<MinionSpawnInfo> parseMinionList(const nlohmann::json& pj) {
            std::vector<MinionSpawnInfo> result;
            if (!pj.contains("minions")) return result;

            for (const auto& mj : pj["minions"]) {
                MinionSpawnInfo minion;
                minion.enemy_type = mj.value("enemy_type", "WalkingDead");
                minion.count = mj.value("count", 1);
                minion.hp = mj.value("hp", 60);
                minion.offset_x = mj.value("offset_x", 3.0f);
                minion.offset_y = mj.value("offset_y", 2.0f);
                result.push_back(minion);
            }
            return result;
        }

        BossPhase parseBossPhase(const nlohmann::json& pj) {
            BossPhase phase;
            phase.hp_threshold = pj.value("hp_threshold", 1.0f);
            phase.name = pj.value("name", "");
            phase.speed_multiplier = pj.value("speed_multiplier", 1.0f);
            phase.damage_multiplier = pj.value("damage_multiplier", 1.0f);
            phase.notification = pj.value("notification", "");
            phase.screen_flash = pj.value("screen_flash", false);

            if (pj.contains("flash_color")) {
                const auto& fc = pj["flash_color"];
                if (fc.is_array() && fc.size() >= 4) {
                    phase.flash_color = {
                        static_cast<unsigned char>(fc[0].get<int>()),
                        static_cast<unsigned char>(fc[1].get<int>()),
                        static_cast<unsigned char>(fc[2].get<int>()),
                        static_cast<unsigned char>(fc[3].get<int>())
                    };
                }
            }

            phase.minions = parseMinionList(pj);
            return phase;
        }

        BossReward parseBossReward(const nlohmann::json& rj) {
            BossReward reward;
            reward.gold = rj.value("gold", 0);
            reward.exp = rj.value("exp", 0);
            if (rj.contains("items")) {
                for (const auto& item_id : rj["items"]) {
                    reward.item_ids.push_back(item_id.get<int>());
                }
            }
            return reward;
        }

        BossIntroDialogue parseBossIntroDialogue(const nlohmann::json& bj) {
            BossIntroDialogue intro;
            if (!bj.contains("intro_dialogue") || !bj["intro_dialogue"].is_object())
                return intro;

            const auto& ij = bj["intro_dialogue"];
            intro.enabled = ij.value("enabled", false);
            intro.required_active_quest = ij.value("required_active_quest", "");
            intro.blocking_active_quest = ij.value("blocking_active_quest", "");
            intro.checkpoint_on_complete = ij.value("checkpoint_on_complete", "");
            intro.final_option = ij.value("final_option", "...");
            intro.show_preview = ij.value("show_preview", false);

            if (ij.contains("lines") && ij["lines"].is_array()) {
                for (const auto& lj : ij["lines"]) {
                    BossDialogueLine line;
                    line.speaker = lj.value("speaker", "");
                    line.text = lj.value("text", "");
                    line.voice_path = lj.value("voice_path", "");
                    intro.lines.push_back(std::move(line));
                }
            }

            intro.enabled = intro.enabled && !intro.lines.empty();
            return intro;
        }

        int getPhaseRestartHp(const BossData& boss, const int phase_index, const int max_hp) {
            if (max_hp <= 0)
                return 1;

            if (phase_index <= 0 || boss.phases.empty())
                return max_hp;

            const int clamped_phase = std::clamp(
                phase_index,
                0,
                static_cast<int>(boss.phases.size()) - 1
            );
            const int threshold_hp = static_cast<int>(std::floor(
                static_cast<float>(max_hp) * boss.phases[clamped_phase].hp_threshold
            ));

            return std::clamp(threshold_hp + 1, 1, max_hp);
        }

        int resolvePhaseIndexForHp(
            const BossData& boss,
            const int current_phase_index,
            const int hp,
            const int max_hp
        ) {
            if (boss.phases.empty() || max_hp <= 0)
                return std::max(0, current_phase_index);

            int result = std::clamp(
                current_phase_index,
                0,
                static_cast<int>(boss.phases.size()) - 1
            );
            const float hp_pct = static_cast<float>(hp) / static_cast<float>(max_hp);
            for (int i = static_cast<int>(boss.phases.size()) - 1; i > result; --i) {
                if (hp_pct <= boss.phases[i].hp_threshold) {
                    result = i;
                    break;
                }
            }

            return result;
        }

    } // namespace anonimowa

    void BossManager::loadFromJson(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            Core::Logger::errorLog("BossManager: Nie udalo sie otworzyc " + path);
            return;
        }

        nlohmann::json data;
        try {
            file >> data;
        }
        catch (const nlohmann::json::parse_error&) {
            Core::Logger::errorLog("BossManager: Blad parsowania JSON w " + path);
            return;
        }

        if (!data.contains("bosses")) return;

        _bosses.clear();
        for (const auto& bj : data["bosses"]) {
            BossData boss;
            boss.id = bj.value("id", "");
            boss.name = bj.value("name", "");
            boss.enemy_type = bj.value("enemy_type", "");
            boss.max_hp = bj.value("max_hp", 1000);
            boss.scale = bj.value("scale", 1.0f);
            boss.music_path = bj.value("music_path", "");
            boss.music_volume = bj.value("music_volume", 0.85f);
            boss.on_player_death = bj.value("on_player_death", "end_fight");
            boss.intro_dialogue = parseBossIntroDialogue(bj);

            // Fazy.
            if (bj.contains("phases")) {
                for (const auto& pj : bj["phases"]) {
                    boss.phases.push_back(parseBossPhase(pj));
                }
            }

            // Domyslna faza, jesli zadna nie zostala zdefiniowana.
            if (boss.phases.empty()) {
                BossPhase default_phase;
                default_phase.hp_threshold = 1.0f;
                default_phase.name = "Faza 1";
                default_phase.speed_multiplier = 1.0f;
                default_phase.damage_multiplier = 1.0f;
                boss.phases.push_back(default_phase);
            }

            // Nagrody.
            if (bj.contains("rewards")) {
                boss.reward = parseBossReward(bj["rewards"]);
            }

            if (!boss.id.empty()) {
                _bosses[boss.id] = boss;
                Core::Logger::debugLog("BossManager: Zaladowano bossa '" + boss.id + "' z " + std::to_string(boss.phases.size()) + " fazami");
            }
        }
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

    BossRuntimeState BossManager::getRuntimeState() const {
        BossRuntimeState state;
        state.active = isFightActive() && _active_boss_data && _active_boss_entity;
        if (!state.active)
            return state;

        state.boss_id = _active_boss_data->id;
        state.current_phase_index = resolvePhaseIndexForHp(
            *_active_boss_data,
            _current_phase_index,
            _active_boss_entity->getHP(),
            _active_boss_entity->getMaxHP()
        );
        state.fight_timer = _fight_timer;
        state.saved_hp = _active_boss_entity->getHP();
        state.max_hp = _active_boss_entity->getMaxHP();
        state.position = {_active_boss_entity->getX(), _active_boss_entity->getY()};
        state.altitude = _active_boss_entity->getAltitude();
        state.spawn_position = _active_boss_spawn_pos;
        state.spawn_altitude = _active_boss_spawn_altitude;
        return state;
    }

    nlohmann::json BossManager::serializeRuntimeState() const {
        const BossRuntimeState state = getRuntimeState();
        nlohmann::json result;
        result["active"] = state.active;
        if (!state.active)
            return result;

        result["boss_id"] = state.boss_id;
        result["current_phase_index"] = state.current_phase_index;
        result["fight_timer"] = state.fight_timer;
        result["saved_hp"] = state.saved_hp;
        result["max_hp"] = state.max_hp;
        result["position"] = {{"x", state.position.x}, {"y", state.position.y}};
        result["altitude"] = state.altitude;
        result["spawn_position"] = {{"x", state.spawn_position.x}, {"y", state.spawn_position.y}};
        result["spawn_altitude"] = state.spawn_altitude;
        return result;
    }

    bool BossManager::applyRuntimeState(const nlohmann::json& data, Core::Engine* engine) {
        if (!data.is_object() || !data.value("active", false))
            return false;

        BossRuntimeState state;
        state.active = true;
        state.boss_id = data.value("boss_id", "");
        state.current_phase_index = data.value("current_phase_index", 0);
        state.fight_timer = data.value("fight_timer", 0.0f);
        state.saved_hp = data.value("saved_hp", 0);
        state.max_hp = data.value("max_hp", 0);

        const auto read_position = [](const nlohmann::json& object) {
            return Vector2{object.value("x", 0.0f), object.value("y", 0.0f)};
        };

        if (data.contains("position") && data["position"].is_object())
            state.position = read_position(data["position"]);
        if (data.contains("spawn_position") && data["spawn_position"].is_object())
            state.spawn_position = read_position(data["spawn_position"]);

        state.altitude = data.value("altitude", 0.0f);
        state.spawn_altitude = data.value("spawn_altitude", 0.0f);

        return restoreRuntimeState(state, engine);
    }

    bool BossManager::restoreRuntimeState(const BossRuntimeState& state, Core::Engine* engine) {
        if (!state.active || state.boss_id.empty() || !engine)
            return false;

        if (isBossDefeated(state.boss_id))
            return false;

        if (isFightActive())
            resetRuntimeState(engine);

        const auto boss_it = _bosses.find(state.boss_id);
        if (boss_it == _bosses.end()) {
            Core::Logger::errorLog("BossManager: Nie mozna odtworzyc nieznanego bossa '" + state.boss_id + "'.");
            return false;
        }

        _active_boss_data = &boss_it->second;
        _active_boss_spawn_pos = state.spawn_position;
        _active_boss_spawn_altitude = state.spawn_altitude;
        const int max_phase_index = _active_boss_data->phases.empty()
            ? 0
            : static_cast<int>(_active_boss_data->phases.size()) - 1;
        _current_phase_index = std::clamp(
            state.current_phase_index,
            0,
            max_phase_index
        );
        _fight_timer = std::max(0.0f, state.fight_timer);
        _phase_flash_timer = 0.0f;

        if (!activateBossFromPool(state.boss_id, engine) && !buildAndActivateBoss(engine)) {
            _active_boss_data = nullptr;
            return false;
        }

        if (!_active_boss_entity) {
            _active_boss_data = nullptr;
            return false;
        }

        const int max_hp = state.max_hp > 0 ? state.max_hp : _active_boss_data->max_hp;
        _active_boss_entity->setMaxHp(max_hp);
        _active_boss_entity->setX(state.position.x);
        _active_boss_entity->setY(state.position.y);
        _active_boss_entity->setAltitude(state.altitude);
        _active_boss_entity->setDormant(false);

        const int restart_hp = getPhaseRestartHp(*_active_boss_data, _current_phase_index, max_hp);
        _active_boss_entity->setHP(restart_hp);

        if (!_active_boss_data->phases.empty())
            applyPhase(_active_boss_data->phases[_current_phase_index], engine);

        startBossMusic(engine);

        Core::Logger::debugLog("BossManager: Odtworzono walke z bossem '" + state.boss_id +
            "' w fazie " + std::to_string(_current_phase_index + 1) +
            " z HP " + std::to_string(restart_hp) + ".");
        return true;
    }

    // -----------------------------------------------------------------------
    // Budowanie encji wroga (wspolne dla bossa i minionow)
    // -----------------------------------------------------------------------

    std::shared_ptr<Entity::Entity> BossManager::buildEnemyEntity(
        const std::string& type, const std::string& name,
        int max_hp, Core::Engine* engine)
    {
        auto player = engine->getPlayer();
        auto* map = engine->getCurrentMap();

        std::shared_ptr<Entity::Entity> entity = nullptr;

        if (type == "Devil") {
            entity = std::shared_ptr<Entity::Entity>(Entity::DevilBuilder()
                .setName(name).setMap(map).setMaxHp(max_hp)
                .setTarget(player).setAudioManager(&engine->getAudioManager())
                .build());
        } else if (type == "WalkingDead") {
            entity = std::shared_ptr<Entity::Entity>(Entity::WalkingDeadBuilder()
                .setName(name).setMap(map).setMaxHp(max_hp)
                .setTarget(player).setAudioManager(&engine->getAudioManager())
                .build());
        } else if (type == "Bandit") {
            entity = std::shared_ptr<Entity::Entity>(Entity::BanditBuilder()
                .setName(name).setMap(map).setMaxHp(max_hp)
                .setTarget(player).setAudioManager(&engine->getAudioManager())
                .build());
        } else if (type == "Frog") {
            entity = std::shared_ptr<Entity::Entity>(Entity::FrogBuilder()
                .setName(name).setMap(map).setEngine(engine).setMaxHp(max_hp)
                .setTarget(player).setAudioManager(&engine->getAudioManager())
                .build());
        }

        return entity;
    }

    std::shared_ptr<Entity::Entity> BossManager::createPreviewEntity(const BossData& boss_data, Core::Engine* engine) {
        if (!engine)
            return nullptr;

        auto entity = buildEnemyEntity(
            boss_data.enemy_type,
            boss_data.name.empty() ? boss_data.id : boss_data.name,
            boss_data.max_hp,
            engine);
        if (!entity)
            return nullptr;

        entity->setType(Entity::EntityType::NPCStatic);
        entity->setFaction(Entity::Faction::None);
        entity->setTarget(nullptr);
        entity->setScale(boss_data.scale);
        return entity;
    }

    // -----------------------------------------------------------------------
    // Preloadowanie
    // -----------------------------------------------------------------------

    void BossManager::clearPreloadedBosses() {
        _minion_pools.clear();
        _boss_pool.clear();
    }

    void BossManager::resetRuntimeState(Core::Engine* engine) {
        removeMinions(engine);

        if (_active_boss_entity && !_active_boss_entity->isDead())
            _active_boss_entity->die();

        _active_boss_data = nullptr;
        _active_boss_entity = nullptr;
        _current_phase_index = 0;
        _fight_timer = 0.0f;
        _phase_flash_timer = 0.0f;
        _minion_pools.clear();
        _boss_pool.clear();
        restoreMusicAfterBoss(engine);
    }

    void BossManager::preloadBossFight(const std::string& boss_id, Core::Engine* engine) {
        if (boss_id.empty() || !engine || !engine->getCurrentMap())
            return;

        const auto boss_it = _bosses.find(boss_id);
        if (boss_it == _bosses.end()) {
            Core::Logger::errorLog("BossManager: Nie mozna preloadowac nieznanego bossa '" + boss_id + "'.");
            return;
        }

        if (preloadBossDefinition(boss_it->second, engine)) {
            Core::Logger::debugLog("BossManager: Preladowano walke z bossem '" + boss_id + "'.");
        }
    }

    bool BossManager::preloadBossDefinition(const BossData& boss, Core::Engine* engine) {
        bool preloaded_anything = false;

        if (!_boss_pool.contains(boss.id)) {
            auto boss_entity = buildEnemyEntity(boss.enemy_type, boss.name, boss.max_hp, engine);
            if (boss_entity) {
                boss_entity->setDormant(true);
                _boss_pool[boss.id] = boss_entity;
                preloaded_anything = true;
            }
        }

        std::map<std::string, int> minion_counts;
        for (const auto& phase : boss.phases) {
            for (const auto& minion : phase.minions) {
                minion_counts[minion.enemy_type] += minion.count;
            }
        }

        for (const auto& [type, count] : minion_counts) {
            auto& pool = _minion_pools[type];
            while (static_cast<int>(pool.size()) < count) {
                auto minion = buildEnemyEntity(type, "Minion", 60, engine);
                if (!minion)
                    break;

                minion->setDormant(true);
                pool.push_back(minion);
                preloaded_anything = true;
            }
        }

        return preloaded_anything;
    }

    // -----------------------------------------------------------------------
    // Aktualizacja
    // -----------------------------------------------------------------------

    void BossManager::update(Core::Engine* engine, float dt) {
        if (!isFightActive()) return;

        _fight_timer += dt;

        // Wygaszanie efektu blysku fazy.
        if (_phase_flash_timer > 0.0f) {
            _phase_flash_timer -= dt;
            if (_phase_flash_timer < 0.0f) _phase_flash_timer = 0.0f;
        }

        // Sprawdzenie smierci bossa.
        if (_active_boss_entity && _active_boss_entity->isDead()) {
            endBossFight(true, engine);
            return;
        }

        if (_active_boss_entity && _active_boss_entity->isDying())
            return;

        // Czyszczenie martwych minionow z listy sledzenia.
        for (auto it = _active_minions.begin(); it != _active_minions.end();) {
            if (!*it || (*it)->isDead()) {
                it = _active_minions.erase(it);
            } else {
                ++it;
            }
        }

        // Sprawdzenie przejsc miedzy fazami.
        checkPhaseTransition(engine);
    }

    // -----------------------------------------------------------------------
    // Startowanie walki
    // -----------------------------------------------------------------------

    bool BossManager::activateBossFromPool(const std::string& boss_id, Core::Engine* engine) {
        auto pool_it = _boss_pool.find(boss_id);
        if (pool_it == _boss_pool.end() || !pool_it->second) return false;

        auto player = engine->getPlayer();
        auto boss_entity = pool_it->second;
        placeEntityAtBossSpawn(boss_entity, engine);
        boss_entity->setMaxHp(_active_boss_data->max_hp);
        boss_entity->setDormant(false);
        boss_entity->setTarget(player);

        auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(boss_entity);
        if (!enemy) return false;

        enemy->setScale(_active_boss_data->scale);
        enemy->setCollider(std::make_unique<Entity::RectangleCollider>(
            enemy.get(),
            _active_boss_data->enemy_type == "Frog" ? 2.0f : 1.2f,
            _active_boss_data->enemy_type == "Frog" ? 2.0f : 1.4f,
            0.0f,
            0.0f));
        enemy->setMap(engine->getCurrentMap());
        _active_boss_entity = enemy;
        engine->getEntityManager().addEntity(_active_boss_entity);
        return true;
    }

    bool BossManager::buildAndActivateBoss(Core::Engine* engine) {
        auto boss_entity = buildEnemyEntity(
            _active_boss_data->enemy_type,
            _active_boss_data->name,
            _active_boss_data->max_hp,
            engine);
        auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(boss_entity);
        if (!enemy) {
            Core::Logger::errorLog("BossManager: Nieznany typ wroga '" + _active_boss_data->enemy_type + "'");
            return false;
        }

        enemy->setScale(_active_boss_data->scale);
        enemy->setCollider(std::make_unique<Entity::RectangleCollider>(
            enemy.get(),
            _active_boss_data->enemy_type == "Frog" ? 2.0f : 1.2f,
            _active_boss_data->enemy_type == "Frog" ? 2.0f : 1.4f,
            0.0f,
            0.0f));
        _active_boss_entity = enemy;
        placeEntityAtBossSpawn(_active_boss_entity, engine);
        engine->getEntityManager().addEntity(_active_boss_entity);
        return true;
    }

    bool BossManager::startBossFight(const std::string& boss_id, Core::Engine* engine) {
        return startBossFightAt(boss_id, engine, false, {0.0f, 0.0f}, 0.0f);
    }

    bool BossManager::startBossFight(
        const std::string& boss_id,
        Core::Engine* engine,
        const Vector2 spawn_pos,
        const float spawn_altitude
    ) {
        return startBossFightAt(boss_id, engine, true, spawn_pos, spawn_altitude);
    }

    bool BossManager::startBossFightAt(
        const std::string& boss_id,
        Core::Engine* engine,
        const bool use_spawn_override,
        const Vector2 spawn_pos,
        const float spawn_altitude
    ) {
        if (isFightActive()) return false;
        if (isBossDefeated(boss_id)) return false;

        auto it = _bosses.find(boss_id);
        if (it == _bosses.end()) {
            Core::Logger::errorLog("BossManager: Boss '" + boss_id + "' nie znaleziony.");
            return false;
        }

        _active_boss_data = &it->second;
        if (use_spawn_override) {
            _active_boss_spawn_pos = spawn_pos;
            _active_boss_spawn_altitude = spawn_altitude;
        } else if (engine && engine->getPlayer()) {
            const auto player = engine->getPlayer();
            _active_boss_spawn_pos = {player->getX(), player->getY()};
            _active_boss_spawn_altitude = player->getAltitude();
        } else {
            _active_boss_spawn_pos = {0.0f, 0.0f};
            _active_boss_spawn_altitude = 0.0f;
        }
        _current_phase_index = 0;
        _fight_timer = 0.0f;
        _phase_flash_timer = 0.0f;
        Core::Logger::debugLog("BossManager: Rozpoczecie walki z bossem: " + _active_boss_data->name);

        // Proba aktywacji z puli preladowanych encji, fallback na budowanie.
        if (!activateBossFromPool(boss_id, engine) && !buildAndActivateBoss(engine)) {
            _active_boss_data = nullptr;
            return false;
        }

        if (!_active_boss_entity) {
            Core::Logger::errorLog("BossManager: Nie udalo sie stworzyc bossa '" + boss_id + "'.");
            _active_boss_data = nullptr;
            return false;
        }

        // Zastosowanie poczatkowej fazy.
        if (!_active_boss_data->phases.empty()) {
            applyPhase(_active_boss_data->phases[0], engine);
        }

        startBossMusic(engine);

        engine->getUIHandler().showNotification("WALKA Z BOSSEM: " + _active_boss_data->name, 4.0f);

        return true;
    }

    Vector3 BossManager::resolveBossSpawnPosition(Core::Engine* engine) const {
        Vector3 spawn_position = {
            _active_boss_spawn_pos.x,
            _active_boss_spawn_altitude,
            _active_boss_spawn_pos.y
        };

        auto* map = engine ? engine->getCurrentMap() : nullptr;
        if (map && map->getNavMesh().isReady()) {
            spawn_position = map->getNavMesh().getClosestWalkablePosition(spawn_position);
        }

        return spawn_position;
    }

    void BossManager::placeEntityAtBossSpawn(
        const std::shared_ptr<Entity::Entity>& entity,
        Core::Engine* engine
    ) const {
        if (!entity)
            return;

        const Vector3 spawn_position = resolveBossSpawnPosition(engine);
        entity->setX(spawn_position.x);
        entity->setY(spawn_position.z);
        entity->setAltitude(spawn_position.y);
    }

    void BossManager::startBossMusic(Core::Engine* engine) {
        if (!engine || !_active_boss_data || _active_boss_data->music_path.empty() || _boss_music_overrode_track)
            return;

        auto& audio = engine->getAudioManager();
        const bool had_previous_music = audio.hasMusic();
        const std::string previous_music_path = audio.getCurrentMusicPath();
        const float previous_music_volume = audio.getCurrentTrackVolume();

        if (audio.playMusic(_active_boss_data->music_path, true, _active_boss_data->music_volume)) {
            _boss_music_overrode_track = true;
            _had_music_before_boss = had_previous_music;
            _music_before_boss_path = previous_music_path;
            _music_before_boss_volume = previous_music_volume;
        }
    }

    void BossManager::restoreMusicAfterBoss(Core::Engine* engine) {
        if (!engine || !_boss_music_overrode_track)
            return;

        auto& audio = engine->getAudioManager();
        if (_had_music_before_boss && !_music_before_boss_path.empty())
            audio.playMusic(_music_before_boss_path, true, _music_before_boss_volume);
        else
            audio.stopMusic();

        _boss_music_overrode_track = false;
        _had_music_before_boss = false;
        _music_before_boss_path.clear();
        _music_before_boss_volume = 1.0f;
    }

    // -----------------------------------------------------------------------
    // Konczenie walki
    // -----------------------------------------------------------------------

    void BossManager::endBossFight(bool victory, Core::Engine* engine) {
        if (!isFightActive()) return;

        if (victory) {
            Core::Logger::debugLog("BossManager: Zwyciestwo! Boss pokonany: " + _active_boss_data->name);
            engine->getUIHandler().showNotification("ZWYCIESTWO! Boss pokonany.", 5.0f);

            // Oznaczenie bossa jako pokonanego.
            _defeated_bosses.insert(_active_boss_data->id);

            // Powiadomienie systemu questow.
            engine->getQuestManager().notifyKill(_active_boss_data->enemy_type);

            // Przyznanie nagrod.
            auto player = engine->getPlayer();
            if (player) {
                player->addExp(_active_boss_data->reward.exp);
                player->addGold(_active_boss_data->reward.gold);
                for (int item_id : _active_boss_data->reward.item_ids) {
                    if (auto item = engine->getItemDatabase().createItem(item_id)) {
                        player->getBackpack().addItem(item);
                    }
                }
            }
        } else {
            Core::Logger::debugLog("BossManager: Porazka. Walka z bossem zakonczona.");
            engine->getUIHandler().showNotification("Walka z bossem zakonczona.", 3.0f);

            // Zabicie encji bossa, zeby arena byla czysta po respawnie.
            if (_active_boss_entity && !_active_boss_entity->isDead()) {
                _active_boss_entity->die();
            }
        }

        removeMinions(engine);
        
        _active_boss_data = nullptr;
        _active_boss_entity = nullptr;
        _current_phase_index = 0;
        _fight_timer = 0.0f;
        _phase_flash_timer = 0.0f;
        _minion_pools.clear();
        restoreMusicAfterBoss(engine);

    }

    // -----------------------------------------------------------------------
    // Fazy
    // -----------------------------------------------------------------------

    void BossManager::checkPhaseTransition(Core::Engine* engine) {
        if (!_active_boss_entity || !_active_boss_data) return;
        if (_active_boss_data->phases.empty()) return;

        float hp_pct = static_cast<float>(_active_boss_entity->getHP()) / 
                       static_cast<float>(_active_boss_entity->getMaxHP());

        // Sprawdzenie, czy nalezy przejsc do pozniejszej fazy.
        for (int i = static_cast<int>(_active_boss_data->phases.size()) - 1; i > _current_phase_index; --i) {
            if (hp_pct <= _active_boss_data->phases[i].hp_threshold) {
                _current_phase_index = i;
                applyPhase(_active_boss_data->phases[i], engine);
                break;
            }
        }
    }

    void BossManager::applyPhase(const BossPhase& phase, Core::Engine* engine) {
        Core::Logger::debugLog("BossManager: Wejscie w faze: " + phase.name);

        // Zastosowanie mnoznikow statystyk bossa.
        if (_active_boss_entity) {
            _active_boss_entity->setSpeedMultiplier(phase.speed_multiplier);
            _active_boss_entity->setDamageMultiplier(phase.damage_multiplier);
        }

        // Wyswietlenie powiadomienia.
        if (!phase.notification.empty()) {
            engine->getUIHandler().showNotification(phase.notification, 3.0f);
        }

        // Efekt blysku ekranu przy przejsciu fazy.
        if (phase.screen_flash) {
            _phase_flash_timer = 0.6f;
            _phase_flash_color = phase.flash_color;
        }

        // Przywolanie minionow.
        if (!phase.minions.empty()) {
            spawnMinions(phase.minions, engine);
        }
    }

    // -----------------------------------------------------------------------
    // Miniony
    // -----------------------------------------------------------------------

    void BossManager::spawnMinions(const std::vector<MinionSpawnInfo>& minions, Core::Engine* engine) {
        if (!_active_boss_entity) return;

        for (const auto& info : minions) {
            for (int i = 0; i < info.count; ++i) {
                // Rozmieszczenie minionow w okregu wokol bossa.
                float angle = (2.0f * 3.14159f / static_cast<float>(info.count)) * i;
                Vector2 spawn_pos = {
                    _active_boss_entity->getX() + info.offset_x * std::cos(angle),
                    _active_boss_entity->getY() + info.offset_y * std::sin(angle)
                };

                std::shared_ptr<Entity::Entity> minion = nullptr;

                // Proba pobrania z puli preladowanych encji.
                if (_minion_pools.count(info.enemy_type) && !_minion_pools[info.enemy_type].empty()) {
                    minion = _minion_pools[info.enemy_type].back();
                    _minion_pools[info.enemy_type].pop_back();
                    minion->setX(spawn_pos.x);
                    minion->setY(spawn_pos.y);
                    minion->setMaxHp(info.hp);
                    minion->setDormant(false);
                } else {
                    // Budowanie w locie, jesli pula jest pusta.
                    minion = buildEnemyEntity(info.enemy_type, "Minion", info.hp, engine);
                    if (minion) {
                        minion->setX(spawn_pos.x);
                        minion->setY(spawn_pos.y);
                    }
                }

                if (minion) {
                    if (auto actor = std::dynamic_pointer_cast<Entity::ActorInterface>(minion))
                        actor->setMap(engine->getCurrentMap());

                    auto* map = engine->getCurrentMap();
                    if (map && map->getNavMesh().isReady()) {
                        const Vector3 snapped_position = map->getNavMesh().getClosestWalkablePosition(
                            {minion->getX(), _active_boss_entity->getAltitude(), minion->getY()});
                        minion->setX(snapped_position.x);
                        minion->setY(snapped_position.z);
                        minion->setAltitude(snapped_position.y);
                    }

                    _active_minions.push_back(minion);
                    engine->getEntityManager().addEntity(minion);
                }
            }
        }

        Core::Logger::debugLog("BossManager: Przywolano " + std::to_string(_active_minions.size()) + " minionow.");
    }

    void BossManager::removeMinions(Core::Engine* engine) {
        for (auto& minion : _active_minions) {
            if (minion && !minion->isDead()) {
                minion->die();
            }
        }
        _active_minions.clear();
        Core::Logger::debugLog("BossManager: Miniony usuniete.");
    }

} // namespace Nawia::Game
