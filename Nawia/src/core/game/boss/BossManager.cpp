#include "BossManager.h"
#include <Engine.h>
#include <Logger.h>
#include <Devil.h>
#include <Bandit.h>
#include <WalkingDead.h>
#include <Collider.h>
#include <json.hpp>
#include <fstream>
#include <iostream>
#include <cmath>
#include <LevelManager.h>

namespace Nawia::Game {

    BossManager::BossManager() {}
    BossManager::~BossManager() {}

    void BossManager::loadFromJson(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            Core::Logger::errorLog("BossManager: Could not open " + path);
            return;
        }

        nlohmann::json data;
        try {
            file >> data;
        }
        catch (const nlohmann::json::parse_error& e) {
            Core::Logger::errorLog("BossManager: JSON parse error in " + path);
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
            boss.level_name = bj.value("level_name", "");
            boss.on_player_death = bj.value("on_player_death", "end_fight");

            // Spawn position
            if (bj.contains("spawn_pos")) {
                const auto& sj = bj["spawn_pos"];
                boss.spawn_pos = { sj.value("x", 0.0f), sj.value("y", 0.0f) };
            }

            // Phases
            if (bj.contains("phases")) {
                for (const auto& pj : bj["phases"]) {
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

                    if (pj.contains("minions")) {
                        for (const auto& mj : pj["minions"]) {
                            MinionSpawnInfo minion;
                            minion.enemy_type = mj.value("enemy_type", "WalkingDead");
                            minion.count = mj.value("count", 1);
                            minion.hp = mj.value("hp", 60);
                            minion.offset_x = mj.value("offset_x", 3.0f);
                            minion.offset_y = mj.value("offset_y", 2.0f);
                            phase.minions.push_back(minion);
                        }
                    }

                    boss.phases.push_back(phase);
                }
            }

            // If no phases defined, create a default phase
            if (boss.phases.empty()) {
                BossPhase default_phase;
                default_phase.hp_threshold = 1.0f;
                default_phase.name = "Phase 1";
                default_phase.speed_multiplier = 1.0f;
                default_phase.damage_multiplier = 1.0f;
                boss.phases.push_back(default_phase);
            }

            // Rewards
            if (bj.contains("rewards")) {
                const auto& rj = bj["rewards"];
                boss.reward.gold = rj.value("gold", 0);
                boss.reward.exp = rj.value("exp", 0);
                if (rj.contains("items")) {
                    for (const auto& item_id : rj["items"]) {
                        boss.reward.item_ids.push_back(item_id.get<int>());
                    }
                }
            }

            if (!boss.id.empty()) {
                _bosses[boss.id] = boss;
                Core::Logger::debugLog("BossManager: Loaded boss '" + boss.id + "' with " + std::to_string(boss.phases.size()) + " phases");
            }
        }
    }

    void BossManager::preloadForLevel(const std::string& level_name, Core::Engine* engine) {
        _minion_pools.clear();
        _boss_pool.clear();

        auto player = engine->getPlayer();
        auto* map = engine->getCurrentMap();
        if (!map) return;

        bool preloaded_anything = false;

        for (const auto& [id, boss] : _bosses) {
            if (boss.level_name != level_name) continue;

            // Preload boss entity
            std::shared_ptr<Entity::Entity> boss_entity = nullptr;
            if (boss.enemy_type == "Devil") {
                boss_entity = std::shared_ptr<Entity::Entity>(Entity::DevilBuilder()
                    .setName(boss.name).setMap(map).setMaxHp(boss.max_hp).setTarget(player).build());
            }

            if (boss_entity) {
                boss_entity->setDormant(true);
                _boss_pool[boss.id] = boss_entity;
                preloaded_anything = true;
            }

            // Preload minions
            std::map<std::string, int> minion_counts;
            for (const auto& phase : boss.phases) {
                for (const auto& minion : phase.minions) {
                    minion_counts[minion.enemy_type] += minion.count;
                }
            }

            for (const auto& pair : minion_counts) {
                const std::string& type = pair.first;
                int count = pair.second;
                auto& pool = _minion_pools[type];
                for (int i = 0; i < count; ++i) {
                    std::shared_ptr<Entity::Entity> minion = nullptr;
                    if (type == "WalkingDead") {
                        minion = std::shared_ptr<Entity::Entity>(Entity::WalkingDeadBuilder()
                            .setName("Minion").setMap(map).setTarget(player).build());
                    } else if (type == "Bandit") {
                        minion = std::shared_ptr<Entity::Entity>(Entity::BanditBuilder()
                            .setName("Minion").setMap(map).setTarget(player).build());
                    } else if (type == "Devil") {
                        minion = std::shared_ptr<Entity::Entity>(Entity::DevilBuilder()
                            .setName("Minion").setMap(map).setTarget(player).build());
                    }

                    if (minion) {
                        minion->setDormant(true);
                        pool.push_back(minion);
                        preloaded_anything = true;
                    }
                }
            }
        }

        if (preloaded_anything) {
            Core::Logger::debugLog("BossManager: Preloaded boss fight resources for level '" + level_name + "'.");
        }
    }

    void BossManager::update(Core::Engine* engine, float dt) {
        if (!isFightActive()) return;

        _fight_timer += dt;

        // Decay phase flash timer
        if (_phase_flash_timer > 0.0f) {
            _phase_flash_timer -= dt;
            if (_phase_flash_timer < 0.0f) _phase_flash_timer = 0.0f;
        }

        // Check if boss died
        if (_active_boss_entity && _active_boss_entity->isDead()) {
            endBossFight(true, engine);
            return;
        }

        // Clean up dead minions from tracking list
        for (auto it = _active_minions.begin(); it != _active_minions.end();) {
            if (!*it || (*it)->isDead()) {
                it = _active_minions.erase(it);
            } else {
                ++it;
            }
        }

        // Check phase transitions
        checkPhaseTransition(engine);
    }

    bool BossManager::startBossFight(const std::string& boss_id, Core::Engine* engine) {
        if (isFightActive()) return false;
        if (isBossDefeated(boss_id)) return false;

        auto it = _bosses.find(boss_id);
        if (it == _bosses.end()) {
            Core::Logger::errorLog("BossManager: Boss '" + boss_id + "' not found.");
            return false;
        }

        _active_boss_data = &it->second;
        _current_phase_index = 0;
        _fight_timer = 0.0f;
        _phase_flash_timer = 0.0f;
        Core::Logger::debugLog("BossManager: Starting boss fight: " + _active_boss_data->name);

        // Spawn boss entity based on type
        if (_boss_pool.count(boss_id) && _boss_pool[boss_id]) {
            auto boss_ent = _boss_pool[boss_id];
            boss_ent->setX(_active_boss_data->spawn_pos.x);
            boss_ent->setY(_active_boss_data->spawn_pos.y);
            boss_ent->setDormant(false);
            
            // set target in case it changed
            boss_ent->setTarget(engine->getPlayer());

            auto interface_ptr = std::dynamic_pointer_cast<Entity::EnemyInterface>(boss_ent);
            if (interface_ptr) {
                interface_ptr->setScale(_active_boss_data->scale);
                // The rectangle collider bounds are specific to devil here, we might need a generic way but for now it's matching Devil.
                interface_ptr->setCollider(std::make_unique<Entity::RectangleCollider>(interface_ptr.get(), 1.2f, 1.4f, 0.0f, 0.0f));
                _active_boss_entity = interface_ptr;
                engine->getEntityManager().addEntity(_active_boss_entity);
            }
        } else {
            // Fallback if not preloaded
            if (_active_boss_data->enemy_type == "Devil") {
                auto player = engine->getPlayer();
                auto* map = engine->getCurrentMap();

                Entity::DevilBuilder builder;
                builder.setPosition(_active_boss_data->spawn_pos)
                       .setName(_active_boss_data->name)
                       .setMaxHp(_active_boss_data->max_hp)
                       .setMap(map);

                if (player) {
                    builder.setTarget(player);
                }
                
                auto devil = builder.build();
                devil->setScale(_active_boss_data->scale);
                devil->setCollider(std::make_unique<Entity::RectangleCollider>(devil.get(), 1.2f, 1.4f, 0.0f, 0.0f));
                _active_boss_entity = std::shared_ptr<Entity::EnemyInterface>(std::move(devil));
                engine->getEntityManager().addEntity(_active_boss_entity);
            } else {
                Core::Logger::errorLog("BossManager: Unknown enemy type '" + _active_boss_data->enemy_type + "'");
                _active_boss_data = nullptr;
                return false;
            }
        }

        // Apply initial phase
        if (!_active_boss_data->phases.empty()) {
            applyPhase(_active_boss_data->phases[0], engine);
        }

        engine->getUIHandler().showNotification("BOSS FIGHT: " + _active_boss_data->name, 4.0f);

        return true;
    }

    void BossManager::endBossFight(bool victory, Core::Engine* engine) {
        if (!isFightActive()) return;

        if (victory) {
            Core::Logger::debugLog("BossManager: Victory! Boss defeated: " + _active_boss_data->name);
            engine->getUIHandler().showNotification("ZWYCIESTWO! Boss pokonany.", 5.0f);

            // Mark boss as defeated so fight can't be re-triggered
            _defeated_bosses.insert(_active_boss_data->id);

            // Notify quest system (use enemy_type as kill name for quest matching)
            engine->getQuestManager().notifyKill(_active_boss_data->enemy_type);

            // Give rewards
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
            Core::Logger::debugLog("BossManager: Defeat. Boss fight ended.");
            engine->getUIHandler().showNotification("Boss fight zakonczona.", 3.0f);

            // Kill the boss entity so the arena is clean after respawn
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

        if (!victory) {
            preloadForLevel(engine->getLevelManager().getCurrentLevelName(), engine);
        }
    }

    void BossManager::checkPhaseTransition(Core::Engine* engine) {
        if (!_active_boss_entity || !_active_boss_data) return;
        if (_active_boss_data->phases.empty()) return;

        float hp_pct = static_cast<float>(_active_boss_entity->getHP()) / 
                       static_cast<float>(_active_boss_entity->getMaxHP());

        // Check if we should advance to a later phase
        for (int i = static_cast<int>(_active_boss_data->phases.size()) - 1; i > _current_phase_index; --i) {
            if (hp_pct <= _active_boss_data->phases[i].hp_threshold) {
                _current_phase_index = i;
                applyPhase(_active_boss_data->phases[i], engine);
                break;
            }
        }
    }

    void BossManager::applyPhase(const BossPhase& phase, Core::Engine* engine) {
        Core::Logger::debugLog("BossManager: Entering phase: " + phase.name);

        // Apply stat multipliers to boss
        if (_active_boss_entity) {
            _active_boss_entity->setSpeedMultiplier(phase.speed_multiplier);
            _active_boss_entity->setDamageMultiplier(phase.damage_multiplier);
        }

        // Show notification
        if (!phase.notification.empty()) {
            engine->getUIHandler().showNotification(phase.notification, 3.0f);
        }

        // Trigger screen flash effect on phase transition
        if (phase.screen_flash) {
            _phase_flash_timer = 0.6f; // 0.6 second flash
            _phase_flash_color = phase.flash_color;
        }

        // Spawn minions
        if (!phase.minions.empty()) {
            spawnMinions(phase.minions, engine);
        }
    }



    void BossManager::spawnMinions(const std::vector<MinionSpawnInfo>& minions, Core::Engine* engine) {
        auto player = engine->getPlayer();
        auto* map = engine->getCurrentMap();
        
        if (!_active_boss_entity) return;

        for (const auto& info : minions) {
            for (int i = 0; i < info.count; ++i) {
                // Spread minions in a circle pattern around the boss
                float angle = (2.0f * 3.14159f / static_cast<float>(info.count)) * i;
                Vector2 spawn_pos = {
                    _active_boss_entity->getX() + info.offset_x * std::cos(angle),
                    _active_boss_entity->getY() + info.offset_y * std::sin(angle)
                };

                std::shared_ptr<Entity::Entity> minion = nullptr;

                // Try to get from pool first to avoid freezing from model loading
                if (_minion_pools.count(info.enemy_type) && !_minion_pools[info.enemy_type].empty()) {
                    minion = _minion_pools[info.enemy_type].back();
                    _minion_pools[info.enemy_type].pop_back();
                    minion->setX(spawn_pos.x);
                    minion->setY(spawn_pos.y);
                    minion->setMaxHp(info.hp);
                    minion->setDormant(false);
                } else {
                    // Fallback to building on the fly if pool is empty
                    if (info.enemy_type == "WalkingDead") {
                        auto built = Entity::WalkingDeadBuilder()
                            .setName("Minion").setPosition(spawn_pos).setMap(map).setMaxHp(info.hp).setTarget(player).build();
                        minion = std::shared_ptr<Entity::Entity>(std::move(built));
                    } else if (info.enemy_type == "Bandit") {
                        auto built = Entity::BanditBuilder()
                            .setName("Minion").setPosition(spawn_pos).setMap(map).setMaxHp(info.hp).setTarget(player).build();
                        minion = std::shared_ptr<Entity::Entity>(std::move(built));
                    } else if (info.enemy_type == "Devil") {
                        auto built = Entity::DevilBuilder()
                            .setName("Minion").setPosition(spawn_pos).setMap(map).setMaxHp(info.hp).setTarget(player).build();
                        minion = std::shared_ptr<Entity::Entity>(std::move(built));
                    }
                }

                if (minion) {
                    _active_minions.push_back(minion);
                    engine->getEntityManager().addEntity(minion);
                }
            }
        }

        Core::Logger::debugLog("BossManager: Spawned " + std::to_string(_active_minions.size()) + " minions.");
    }

    void BossManager::removeMinions(Core::Engine* engine) {
        for (auto& minion : _active_minions) {
            if (minion && !minion->isDead()) {
                minion->die();
            }
        }
        _active_minions.clear();
        Core::Logger::debugLog("BossManager: Minions removed.");
    }

} // namespace Nawia::Game
