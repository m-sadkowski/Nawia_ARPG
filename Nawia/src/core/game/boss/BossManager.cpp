#include "BossManager.h"

#include <BossDefinitionLoader.h>
#include <BossDialogueBuilder.h>
#include <BossEnemyFactory.h>
#include <BossMinionSpawner.h>
#include <BossPhaseMath.h>
#include <BossPreloader.h>
#include <BossRuntimeSerializer.h>
#include <Collider.h>
#include <Engine.h>
#include <EnemyInterface.h>
#include <Logger.h>
#include <Map.h>
#include <Player.h>
#include <QuestManager.h>
#include <UIHandler.h>
#include <json.hpp>

#include <algorithm>

namespace Nawia::Game {

    BossManager::BossManager() {}
    BossManager::~BossManager() {}

    namespace {

        void applyConfiguredScale(const std::shared_ptr<Entity::Entity>& entity, const float scale) {
            if (entity && scale > 0.0f)
                entity->setScale(scale);
        }

    } // namespace anonimowa

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

    BossRuntimeState BossManager::getRuntimeState() const {
        BossRuntimeState state;
        state.active = isFightActive() && _active_boss_data && _active_boss_entity;
        if (!state.active)
            return state;

        state.boss_id = _active_boss_data->id;
        state.current_phase_index = BossPhaseMath::resolvePhaseIndexForHp(
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
        return BossRuntimeSerializer::toJson(getRuntimeState());
    }

    bool BossManager::applyRuntimeState(const nlohmann::json& data, Core::Engine* engine) {
        BossRuntimeState state;
        if (!BossRuntimeSerializer::fromJson(data, state))
            return false;

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

        const int restart_hp = BossPhaseMath::restartHp(*_active_boss_data, _current_phase_index, max_hp);
        _active_boss_entity->setHP(restart_hp);

        if (!_active_boss_data->phases.empty())
            applyPhase(_active_boss_data->phases[_current_phase_index], engine);

        startBossMusic(engine);

        Core::Logger::debugLog("BossManager: Odtworzono walke z bossem '" + state.boss_id +
            "' w fazie " + std::to_string(_current_phase_index + 1) +
            " z HP " + std::to_string(restart_hp) + ".");
        return true;
    }

    std::shared_ptr<Entity::Entity> BossManager::createPreviewEntity(const BossData& boss_data, Core::Engine* engine) {
        if (!engine)
            return nullptr;

        auto entity = BossEnemyFactory::create(
            boss_data.enemy_type,
            boss_data.name.empty() ? boss_data.id : boss_data.name,
            boss_data.max_hp,
            engine);
        if (!entity)
            return nullptr;

        entity->setType(Entity::EntityType::NPCStatic);
        entity->setFaction(Entity::Faction::None);
        entity->setTarget(nullptr);
        applyConfiguredScale(entity, boss_data.scale);
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

        if (_active_boss_entity)
            _active_boss_entity->setHealToFullOnKill(false);

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

        if (BossPreloader::preloadBossDefinition(boss_it->second, engine, _boss_pool, _minion_pools)) {
            Core::Logger::debugLog("BossManager: Preladowano walke z bossem '" + boss_id + "'.");
        }
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

        applyConfiguredScale(enemy, _active_boss_data->scale);
        enemy->setCollider(std::make_unique<Entity::RectangleCollider>(
            enemy.get(),
            _active_boss_data->enemy_type == "Frog" ? 2.0f : 1.2f,
            _active_boss_data->enemy_type == "Frog" ? 2.0f : 1.4f,
            0.0f,
            0.0f));
        enemy->setMap(engine->getCurrentMap());
        enemy->setHealToFullOnKill(true);
        _active_boss_entity = enemy;
        engine->getEntityManager().addEntity(_active_boss_entity);
        return true;
    }

    bool BossManager::buildAndActivateBoss(Core::Engine* engine) {
        auto boss_entity = BossEnemyFactory::create(
            _active_boss_data->enemy_type,
            _active_boss_data->name,
            _active_boss_data->max_hp,
            engine);
        auto enemy = std::dynamic_pointer_cast<Entity::EnemyInterface>(boss_entity);
        if (!enemy) {
            Core::Logger::errorLog("BossManager: Nieznany typ wroga '" + _active_boss_data->enemy_type + "'");
            return false;
        }

        applyConfiguredScale(enemy, _active_boss_data->scale);
        enemy->setCollider(std::make_unique<Entity::RectangleCollider>(
            enemy.get(),
            _active_boss_data->enemy_type == "Frog" ? 2.0f : 1.2f,
            _active_boss_data->enemy_type == "Frog" ? 2.0f : 1.4f,
            0.0f,
            0.0f));
        enemy->setHealToFullOnKill(true);
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
        if (const auto player = engine->getPlayer())
            player->setRespawnPoint({player->getX(), player->getY()});
        engine->saveGameToActiveSlot();

        return true;
    }

    bool BossManager::retryActiveBossFight(Core::Engine* engine) {
        if (!isFightActive() || !_active_boss_data || !engine)
            return false;

        removeMinions(engine);

        if (!_active_boss_entity) {
            if (!activateBossFromPool(_active_boss_data->id, engine) && !buildAndActivateBoss(engine)) {
                _active_boss_data = nullptr;
                return false;
            }
        }

        if (!_active_boss_entity)
            return false;

        _current_phase_index = 0;
        _fight_timer = 0.0f;
        _phase_flash_timer = 0.0f;

        placeEntityAtBossSpawn(std::dynamic_pointer_cast<Entity::Entity>(_active_boss_entity), engine);
        _active_boss_entity->setMaxHp(_active_boss_data->max_hp);
        _active_boss_entity->setHP(_active_boss_data->max_hp);
        _active_boss_entity->setTarget(engine->getPlayer());
        _active_boss_entity->setDormant(false);
        _active_boss_entity->setHealToFullOnKill(true);

        if (!_active_boss_data->phases.empty())
            applyPhase(_active_boss_data->phases[0], engine);

        engine->getUIHandler().showNotification("Walka z bossem zaczyna sie od nowa.", 3.0f);
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

        const std::string victory_dialogue_key = _active_boss_data ? _active_boss_data->victory_dialogue_key : "";
        const std::string checkpoint_on_victory = _active_boss_data ? _active_boss_data->checkpoint_on_victory : "";
        const std::shared_ptr<Entity::Entity> defeated_boss_entity = _active_boss_entity;

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

        if (victory && engine) {
            engine->cancelPlayerAction();
            if (auto player = engine->getPlayer())
                player->clearControlLocks();
        }

        if (_active_boss_entity)
            _active_boss_entity->setHealToFullOnKill(false);
        
        _active_boss_data = nullptr;
        _active_boss_entity = nullptr;
        _current_phase_index = 0;
        _fight_timer = 0.0f;
        _phase_flash_timer = 0.0f;
        _minion_pools.clear();
        restoreMusicAfterBoss(engine);

        if (victory && engine && !victory_dialogue_key.empty()) {
            DialogueTree tree = BossDialogueBuilder::buildFromNpcConfig(victory_dialogue_key);
            if (tree.getNode(0)) {
                engine->getUIHandler().openDialogueFacing(tree, defeated_boss_entity, 0, [engine, checkpoint_on_victory, defeated_boss_entity](const int, const bool completed) {
                    if (defeated_boss_entity)
                        defeated_boss_entity->setDormant(true);

                    if (!completed || checkpoint_on_victory.empty())
                        return;

                    engine->getQuestManager().notifyCheckpointReached(checkpoint_on_victory);
                    engine->getQuestManager().update(engine);
                });
                return;
            }
        }

        if (victory && engine && !checkpoint_on_victory.empty()) {
            engine->getQuestManager().notifyCheckpointReached(checkpoint_on_victory);
            engine->getQuestManager().update(engine);
        }

        if (victory && defeated_boss_entity)
            defeated_boss_entity->setDormant(true);

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
        BossMinionSpawner::spawn(minions, _active_boss_entity, _minion_pools, _active_minions, engine);
    }

    void BossManager::removeMinions(Core::Engine* engine) {
        (void)engine;
        BossMinionSpawner::remove(_active_minions);
    }

} // namespace Nawia::Game
