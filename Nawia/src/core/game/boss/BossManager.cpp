#include "BossManager.h"
#include <Engine.h>
#include <Logger.h>
#include <Devil.h>
#include <Bandit.h>
#include <WalkingDead.h>
#include <BossWall.h>
#include <Collider.h>
#include <json.hpp>
#include <fstream>
#include <iostream>

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

            // Arena
            if (bj.contains("arena")) {
                const auto& aj = bj["arena"];
                boss.arena = { 
                    aj.value("x", 0.0f), 
                    aj.value("y", 0.0f), 
                    aj.value("width", 10.0f), 
                    aj.value("height", 10.0f) 
                };
                
                // Wall config
                boss.wall_config.thickness = aj.value("wall_thickness", 2.0f);
                if (aj.contains("wall_color")) {
                    const auto& wc = aj["wall_color"];
                    if (wc.is_array() && wc.size() >= 4) {
                        boss.wall_config.color = {
                            static_cast<unsigned char>(wc[0].get<int>()),
                            static_cast<unsigned char>(wc[1].get<int>()),
                            static_cast<unsigned char>(wc[2].get<int>()),
                            static_cast<unsigned char>(wc[3].get<int>())
                        };
                    }
                }
            }

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

    void BossManager::update(Core::Engine* engine, float dt) {
        if (!isFightActive()) return;

        _fight_timer += dt;

        // Check if boss died
        if (_active_boss_entity && _active_boss_entity->isDead()) {
            endBossFight(true, engine);
            return;
        }

        // Check phase transitions
        checkPhaseTransition(engine);
    }

    bool BossManager::startBossFight(const std::string& boss_id, Core::Engine* engine) {
        if (isFightActive()) return false;

        auto it = _bosses.find(boss_id);
        if (it == _bosses.end()) {
            Core::Logger::errorLog("BossManager: Boss '" + boss_id + "' not found.");
            return false;
        }

        _active_boss_data = &it->second;
        _current_phase_index = 0;
        _fight_timer = 0.0f;
        Core::Logger::debugLog("BossManager: Starting boss fight: " + _active_boss_data->name);

        // Spawn boss entity based on type
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

        // Apply initial phase
        if (!_active_boss_data->phases.empty()) {
            applyPhase(_active_boss_data->phases[0], engine);
        }

        // Spawn arena walls
        spawnWalls(_active_boss_data->arena, _active_boss_data->wall_config, engine);

        engine->getUIHandler().showNotification("BOSS FIGHT: " + _active_boss_data->name, 4.0f);

        return true;
    }

    void BossManager::endBossFight(bool victory, Core::Engine* engine) {
        if (!isFightActive()) return;

        if (victory) {
            Core::Logger::debugLog("BossManager: Victory! Boss defeated: " + _active_boss_data->name);
            engine->getUIHandler().showNotification("ZWYCIESTWO! Boss pokonany.", 5.0f);

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
        }

        removeMinions(engine);
        removeWalls(engine);
        
        _active_boss_data = nullptr;
        _active_boss_entity = nullptr;
        _current_phase_index = 0;
        _fight_timer = 0.0f;
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

        // Spawn minions
        if (!phase.minions.empty()) {
            spawnMinions(phase.minions, engine);
        }
    }

    void BossManager::spawnWalls(const Rectangle& arena, const BossWallConfig& config, Core::Engine* engine) {
        _active_walls.clear();
        
        float thickness = config.thickness;
        Color color = config.color;
        
        // North wall
        auto wallN = std::make_shared<Entity::BossWall>(arena.x + arena.width / 2.0f, arena.y, arena.width + thickness, thickness, color);
        _active_walls.push_back(wallN);
        engine->getEntityManager().addEntity(wallN);
        
        // South wall
        auto wallS = std::make_shared<Entity::BossWall>(arena.x + arena.width / 2.0f, arena.y + arena.height, arena.width + thickness, thickness, color);
        _active_walls.push_back(wallS);
        engine->getEntityManager().addEntity(wallS);
        
        // West wall
        auto wallW = std::make_shared<Entity::BossWall>(arena.x, arena.y + arena.height / 2.0f, thickness, arena.height, color);
        _active_walls.push_back(wallW);
        engine->getEntityManager().addEntity(wallW);
        
        // East wall
        auto wallE = std::make_shared<Entity::BossWall>(arena.x + arena.width, arena.y + arena.height / 2.0f, thickness, arena.height, color);
        _active_walls.push_back(wallE);
        engine->getEntityManager().addEntity(wallE);

        Core::Logger::debugLog("BossManager: Spawned arena walls.");
    }

    void BossManager::removeWalls(Core::Engine* engine) {
        for (auto& wall : _active_walls) {
            wall->die();
        }
        _active_walls.clear();
        Core::Logger::debugLog("BossManager: Arena walls removed.");
    }

    void BossManager::spawnMinions(const std::vector<MinionSpawnInfo>& minions, Core::Engine* engine) {
        auto player = engine->getPlayer();
        auto* map = engine->getCurrentMap();
        
        if (!_active_boss_entity) return;

        for (const auto& info : minions) {
            for (int i = 0; i < info.count; ++i) {
                // Alternate spawn positions around boss
                float sign_x = (i % 2 == 0) ? 1.0f : -1.0f;
                float sign_y = (i / 2 % 2 == 0) ? 1.0f : -1.0f;
                Vector2 spawn_pos = {
                    _active_boss_entity->getX() + info.offset_x * sign_x * (i + 1),
                    _active_boss_entity->getY() + info.offset_y * sign_y
                };

                std::shared_ptr<Entity::Entity> minion = nullptr;

                if (info.enemy_type == "WalkingDead") {
                    auto built = Entity::WalkingDeadBuilder()
                        .setName("Minion")
                        .setPosition(spawn_pos)
                        .setMap(map)
                        .setMaxHp(info.hp)
                        .setTarget(player)
                        .build();
                    minion = std::shared_ptr<Entity::Entity>(std::move(built));
                } else if (info.enemy_type == "Bandit") {
                    auto built = Entity::BanditBuilder()
                        .setName("Minion")
                        .setPosition(spawn_pos)
                        .setMap(map)
                        .setMaxHp(info.hp)
                        .setTarget(player)
                        .build();
                    minion = std::shared_ptr<Entity::Entity>(std::move(built));
                } else if (info.enemy_type == "Devil") {
                    auto built = Entity::DevilBuilder()
                        .setName("Minion")
                        .setPosition(spawn_pos)
                        .setMap(map)
                        .setMaxHp(info.hp)
                        .setTarget(player)
                        .build();
                    minion = std::shared_ptr<Entity::Entity>(std::move(built));
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
