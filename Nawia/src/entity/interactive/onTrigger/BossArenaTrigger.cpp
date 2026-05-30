#include "BossArenaTrigger.h"

#include <Collider.h>
#include <Dialogue.h>
#include <Engine.h>
#include <BossManager.h>
#include <Player.h>
#include <QuestManager.h>

namespace Nawia::Entity {

    BossArenaTrigger::BossArenaTrigger(const std::string& boss_id, float x, float y, float width, float height)
        : InteractiveTrigger("BossTrigger_" + boss_id, x, y, nullptr, 1), _boss_id(boss_id)
    {
        setType(EntityType::Trigger);
        setFaction(Faction::None);
        setCollider(std::make_unique<RectangleCollider>(this, width, height, 0.0f, 0.0f));
    }

    void BossArenaTrigger::onTriggerEnter(Entity& other) {
        if (other.getFaction() == Faction::Player) {
            if (auto* player = dynamic_cast<Player*>(&other)) {
                if (player->getEngine()) {
                    auto* engine = player->getEngine();
                    auto& boss_mgr = engine->getBossManager();
                    if (boss_mgr.isFightActive() || _intro_dialogue_open) return;

                    const auto* boss_data = getBossData(engine);
                    if (boss_data && shouldRunIntro(engine, *boss_data)) {
                        showBossPreview(engine, *boss_data);
                        openIntroDialogue(engine, *boss_data);
                        return;
                    }

                    startBoss(engine);
                }
            }
        }
    }

    const Game::BossData* BossArenaTrigger::getBossData(Core::Engine* engine) const {
        if (!engine)
            return nullptr;

        const auto& bosses = engine->getBossManager().getAllBosses();
        const auto boss_it = bosses.find(_boss_id);
        return boss_it != bosses.end() ? &boss_it->second : nullptr;
    }

    bool BossArenaTrigger::shouldRunIntro(Core::Engine* engine, const Game::BossData& boss_data) const {
        if (!engine || _intro_completed || !boss_data.intro_dialogue.enabled)
            return false;

        const auto& intro = boss_data.intro_dialogue;
        if (!intro.required_active_quest.empty()) {
            const auto* required_quest = engine->getQuestManager().getQuest(intro.required_active_quest);
            if (!required_quest || !required_quest->isActive())
                return false;
        }

        if (!intro.blocking_active_quest.empty()) {
            const auto* blocking_quest = engine->getQuestManager().getQuest(intro.blocking_active_quest);
            if (blocking_quest && blocking_quest->isActive())
                return false;
        }

        return true;
    }

    void BossArenaTrigger::showBossPreview(Core::Engine* engine, const Game::BossData& boss_data) {
        if (!engine || !boss_data.intro_dialogue.show_preview)
            return;

        if (const auto existing = _preview_boss.lock()) {
            existing->setDormant(false);
            return;
        }

        auto preview = engine->getBossManager().createPreviewEntity(boss_data, engine);
        if (!preview)
            return;

        preview->setX(getCenter().x);
        preview->setY(getCenter().y);
        preview->setAltitude(getAltitude());
        _preview_boss = preview;
        engine->spawnEntity(preview);
    }

    void BossArenaTrigger::hideBossPreview() {
        if (const auto preview = _preview_boss.lock())
            preview->setDormant(true);
    }

    void BossArenaTrigger::openIntroDialogue(Core::Engine* engine, const Game::BossData& boss_data) {
        if (!engine)
            return;

        Game::DialogueTree tree;
        const auto& intro = boss_data.intro_dialogue;
        for (size_t i = 0; i < intro.lines.size(); ++i) {
            Game::DialogueNode node;
            node.id = static_cast<int>(i);
            node.speaker_name = intro.lines[i].speaker;
            node.text = intro.lines[i].text;
            node.voice_path = intro.lines[i].voice_path;

            Game::DialogueOption option;
            option.text = (i + 1 < intro.lines.size()) ? "..." : intro.final_option;
            option.next_node_id = (i + 1 < intro.lines.size()) ? static_cast<int>(i + 1) : -1;
            node.options.push_back(option);
            tree.addNode(node);
        }

        _intro_dialogue_open = true;
        engine->getUIHandler().openDialogue(tree, 0, [this, engine, checkpoint = intro.checkpoint_on_complete](const int, const bool completed) {
            _intro_dialogue_open = false;
            if (!completed)
                return;

            _intro_completed = true;
            if (!checkpoint.empty()) {
                engine->getQuestManager().notifyCheckpointReached(checkpoint);
                engine->getQuestManager().update(engine);
            }
            startBoss(engine);
        });
    }

    void BossArenaTrigger::startBoss(Core::Engine* engine) {
        if (!engine)
            return;

        hideBossPreview();
        engine->getBossManager().startBossFight(_boss_id, engine, getCenter(), getAltitude());
    }

    void BossArenaTrigger::update(float dt) {
        Entity::update(dt);
    }

    void BossArenaTrigger::render(const Camera3D& camera) {
        if (DebugColliders && _collider) {
            auto* rect_collider = dynamic_cast<RectangleCollider*>(_collider.get());
            if (rect_collider) {
                Vector2 center = rect_collider->getPosition();
                float w = rect_collider->getWidth();
                float h = rect_collider->getHeight();
                DrawCubeWires(Vector3{center.x, getAltitude() + 0.1f, center.y}, w, 0.2f, h, PURPLE);
            }
        }
    }

    float BossArenaTrigger::getInteractionRange() {
        return 0.0f;
    }

} // namespace Nawia::Entity
