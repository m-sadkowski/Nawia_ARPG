#include "BossManager.h"
#include "BossManagerInternal.h"

#include <BossEnemyFactory.h>
#include <Collider.h>
#include <Engine.h>
#include <EnemyInterface.h>
#include <Entity.h>
#include <Logger.h>
#include <Map.h>
#include <Player.h>
#include <UIHandler.h>

#include <memory>

namespace Nawia::Game {

    namespace {

        void configureBossEnemy(
            const std::shared_ptr<Entity::EnemyInterface>& enemy,
            const BossData& boss_data)
        {
            BossManagerDetail::applyConfiguredScale(enemy, boss_data.scale);
            enemy->setCollider(std::make_unique<Entity::RectangleCollider>(
                enemy.get(),
                boss_data.enemy_type == "Frog" ? 2.0f : 1.2f,
                boss_data.enemy_type == "Frog" ? 2.0f : 1.4f,
                0.0f,
                0.0f));
            enemy->setHealToFullOnKill(true);
        }

    } // namespace

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

        configureBossEnemy(enemy, *_active_boss_data);
        enemy->setMap(engine->getCurrentMap());
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

        configureBossEnemy(enemy, *_active_boss_data);
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

        if (!activateBossFromPool(boss_id, engine) && !buildAndActivateBoss(engine)) {
            _active_boss_data = nullptr;
            return false;
        }

        if (!_active_boss_entity) {
            Core::Logger::errorLog("BossManager: Nie udalo sie stworzyc bossa '" + boss_id + "'.");
            _active_boss_data = nullptr;
            return false;
        }

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

} // namespace Nawia::Game
