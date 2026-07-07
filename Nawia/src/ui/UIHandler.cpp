#include "UIHandler.h"

#include <BossManager.h>
#include <GlobalScaling.h>
#include <InteractiveClickable.h>
#include <LevelManager.h>
#include <LevelSelectMenu.h>
#include <Player.h>
#include <ResourceManager.h>
#include <Settings.h>
#include <SettingsMenu.h>
#include <StatsUI.h>

#include <raymath.h>

#include <algorithm>

namespace Nawia::UI
{
    UIHandler::UIHandler() : _player(nullptr), _entity_manager(nullptr) {}

    UIHandler::~UIHandler()
    {
        UnloadFont(_font);
    }

    void UIHandler::initialize(
        const std::shared_ptr<Entity::Player>& player,
        Core::EntityManager* entity_manager,
        Core::ResourceManager& resource_manager,
        Game::QuestManager* quest_manager,
        const Core::Settings* settings)
    {
        _player = player;
        _entity_manager = entity_manager;
        _quest_manager = quest_manager;
        _settings = settings;

        _font = LoadFontEx("assets/fonts/slavic_font.ttf", Core::GlobalScaling::scaledInt(300), nullptr, 0);
        GenTextureMipmaps(&_font.texture);
        SetTextureFilter(_font.texture, TEXTURE_FILTER_TRILINEAR);

        _main_menu_background = resource_manager.getTexture("assets/textures/main_menu.png");
        smoothUiTexture(_main_menu_background);
        _menu_btn_idle = resource_manager.getTexture("assets/textures/ui/button.png");
        smoothUiTexture(_menu_btn_idle);
        _ability_bar_frame = resource_manager.getTexture("assets/textures/ui/ability_bar.png");
        _empty_ability_icon = resource_manager.getTexture("assets/textures/icons/empty_ability_icon.png");
        _food_icon = resource_manager.getTexture("assets/textures/icons/food_icon.png");
        _hp_orb_frame = resource_manager.getTexture("assets/textures/ui/hp_orb.png");
        _level_orb_frame = resource_manager.getTexture("assets/textures/ui/level_orb.png");
        smoothUiTexture(_ability_bar_frame);
        smoothUiTexture(_food_icon);
        smoothUiTexture(_hp_orb_frame);
        smoothUiTexture(_level_orb_frame);

        _inventory_ui = std::make_unique<InventoryUI>();
        _inventory_ui->loadResources(resource_manager);

        _chest_ui = std::make_unique<ChestUI>();
        _chest_ui->loadResources(resource_manager);
        _stats_ui = std::make_unique<StatsUI>(_player);
        _stats_ui->loadResources(resource_manager);
        _quest_ui = std::make_unique<QuestUI>();
        _quest_ui->loadResources(resource_manager);

        _previous_hp = _player ? _player->getHP() : 0;
        _visual_hp_percent = 1.0f;
        _visual_exp_percent = 0.0f;
    }

    void UIHandler::setPlayer(const std::shared_ptr<Entity::Player>& player)
    {
        _player = player;
        if (_stats_ui)
            _stats_ui->setPlayer(player);

        _previous_hp = _player ? _player->getHP() : 0;
        _visual_hp_percent = 1.0f;
        _visual_exp_percent = 0.0f;
    }

    void UIHandler::openDialogue(const Game::DialogueTree& tree, const int start_node_id, std::function<void(int, bool)> on_close)
    {
        _dialogueUI.open(tree, start_node_id, std::move(on_close));
    }

    void UIHandler::openDialogueFacing(
        const Game::DialogueTree& tree,
        const std::shared_ptr<Entity::Entity>& speaker,
        const int start_node_id,
        std::function<void(int, bool)> on_close)
    {
        if (_player && speaker) {
            _player->stop();
            _player->rotateTowardsCenter(speaker->getCenter().x, speaker->getCenter().y);
        }

        openDialogue(tree, start_node_id, std::move(on_close));
    }

    void UIHandler::triggerLocationBanner()
    {
        _location_banner_timer = 5.0f;
        if (_level_manager)
            _last_location_name = _level_manager->getCurrentLocationName();
    }

    void UIHandler::onLevelLoaded()
    {
        _location_banner_timer = 6.0f;
        if (_level_manager)
            _last_location_name = _level_manager->getCurrentLocationName();
        _ignore_next_dt = true;
    }

    void UIHandler::update(float delta_time)
    {
        if (_player)
        {
            const float target_hp = std::clamp(static_cast<float>(_player->getHP()) / _player->getMaxHP(), 0.0f, 1.0f);
            const float target_exp = std::clamp(static_cast<float>(_player->getExp()) / std::max(1, _player->getExpToNextLvl()), 0.0f, 1.0f);

            _visual_hp_percent = Lerp(_visual_hp_percent, target_hp, delta_time * 5.0f);
            _visual_exp_percent = Lerp(_visual_exp_percent, target_exp, delta_time * 3.0f);

            if (_player->getHP() < _previous_hp)
                _damage_flash_timer = 0.3f;
            _previous_hp = _player->getHP();
        }

        if (_damage_flash_timer > 0.0f)
            _damage_flash_timer = std::max(0.0f, _damage_flash_timer - delta_time);

        updateMenuHoverTimers(delta_time);

        for (auto iterator = _notifications.begin(); iterator != _notifications.end();)
        {
            iterator->timer -= delta_time;
            if (iterator->timer <= 0.0f)
                iterator = _notifications.erase(iterator);
            else
                ++iterator;
        }

        float effective_delta_time = (delta_time > 0.1f) ? 0.016f : delta_time;
        if (_ignore_next_dt)
        {
            effective_delta_time = 0.0f;
            _ignore_next_dt = false;
        }

        if (_level_manager && _location_banner_timer <= 0.0f)
        {
            const std::string& current_location = _level_manager->getCurrentLocationName();
            if (current_location != _last_location_name && !current_location.empty())
            {
                _last_location_name = current_location;
                triggerLocationBanner();
            }
        }

        if (_location_banner_timer > 0.0f)
            _location_banner_timer -= effective_delta_time;
    }

    void UIHandler::render(const Core::GameCamera& camera, const Game::BossManager* boss_manager)
    {
        if (!_player || !_entity_manager)
            return;

        renderPlayerExperienceBar();
        renderPlayerHealthBar();
        renderPlayerStatusEffects();
        renderPlayerAbilityBar();
        renderCombatEntityHealthBars(camera, boss_manager);
        renderBossHealthBar(boss_manager);

        if (boss_manager && boss_manager->getPhaseFlashTimer() > 0.0f)
        {
            const float flash_alpha = std::clamp(boss_manager->getPhaseFlashTimer() / 0.6f, 0.0f, 1.0f) * 0.5f;
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(boss_manager->getPhaseFlashColor(), flash_alpha));
        }

        renderLocationInfo();

        if (_damage_flash_timer > 0.0f)
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RED, (_damage_flash_timer / 0.3f) * 0.4f));

        if (_settings && _settings->show_fps)
        {
            const char* fps_text = TextFormat("%d FPS", GetFPS());
            const float font_size = Core::GlobalScaling::scaled(18.0f);
            const float margin = Core::GlobalScaling::scaled(20.0f);
            const Vector2 text_size = MeasureTextEx(_font, fps_text, font_size, 1.0f);
            DrawTextEx(_font, fps_text, {static_cast<float>(GetScreenWidth()) - text_size.x - margin, margin}, font_size, 1.0f, WHITE);
        }

        float current_notify_y = 10.0f;
        for (const auto& notification : _notifications)
        {
            const Vector2 text_size = MeasureTextEx(_font, notification.text.c_str(), Core::GlobalScaling::scaled(FONT_SIZE_BUTTON), 1.0f);
            const float pos_x = GetScreenWidth() - text_size.x - 20.0f;

            DrawRectangle(static_cast<int>(pos_x - 5), static_cast<int>(current_notify_y - 5), static_cast<int>(text_size.x + 10), static_cast<int>(text_size.y + 10), Fade(BLACK, 0.7f));
            DrawRectangleLines(static_cast<int>(pos_x - 5), static_cast<int>(current_notify_y - 5), static_cast<int>(text_size.x + 10), static_cast<int>(text_size.y + 10), WHITE);
            DrawTextEx(_font, notification.text.c_str(), {pos_x, current_notify_y}, Core::GlobalScaling::scaled(FONT_SIZE_BUTTON), 1.0f, WHITE);

            current_notify_y += text_size.y + 20.0f;
        }

        _dialogueUI.render(_font);

        if (_is_inventory_open)
        {
            _inventory_ui->render(_font, *_player);
            if (_current_container)
            {
                if (const auto* container_inventory = _current_container->getInventory())
                    _chest_ui->render(*container_inventory, _font);
                if (_stats_ui)
                    _stats_ui->render(Core::GlobalScaling::scaled(50.0f), Core::GlobalScaling::scaled(610.0f), _font);
            }
            else if (_stats_ui)
            {
                _stats_ui->render(Core::GlobalScaling::scaled(50.0f), Core::GlobalScaling::scaled(610.0f), _font);
            }
        }

        if (_is_quest_ui_open)
            _quest_ui->render(_font, _quest_manager);
    }

    void UIHandler::renderDialogueOnly()
    {
        _dialogueUI.render(_font);
    }

    void UIHandler::showNotification(const std::string& text, const float duration)
    {
        _notifications.push_back({text, duration, duration});
    }
} // namespace Nawia::UI
