#include "BossManager.h"

#include <BossDialogueBuilder.h>
#include <Engine.h>
#include <EnemyInterface.h>
#include <Entity.h>
#include <Logger.h>
#include <Player.h>
#include <QuestManager.h>
#include <UIHandler.h>

namespace Nawia::Game {

    void BossManager::endBossFight(bool victory, Core::Engine* engine) {
        if (!isFightActive()) return;

        const std::string victory_dialogue_key = _active_boss_data ? _active_boss_data->victory_dialogue_key : "";
        const std::string checkpoint_on_victory = _active_boss_data ? _active_boss_data->checkpoint_on_victory : "";
        const std::shared_ptr<Entity::Entity> defeated_boss_entity = _active_boss_entity;

        if (victory) {
            Core::Logger::debugLog("BossManager: Zwyciestwo! Boss pokonany: " + _active_boss_data->name);
            engine->getUIHandler().showNotification("ZWYCIESTWO! Boss pokonany.", 5.0f);

            _defeated_bosses.insert(_active_boss_data->id);
            engine->getQuestManager().notifyKill(_active_boss_data->enemy_type);

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

} // namespace Nawia::Game
