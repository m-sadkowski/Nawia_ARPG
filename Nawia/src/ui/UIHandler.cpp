#include "UIHandler.h"

#include <Ability.h>
#include <Camera.h>
#include <Collider.h>
#include <Constants.h>
#include <Entity.h>
#include <EntityManager.h>
#include <GlobalScaling.h>
#include <InteractiveClickable.h>
#include <LevelSelectMenu.h>
#include <LevelManager.h>
#include <Player.h>
#include <QuestManager.h>
#include <ResourceManager.h>
#include <Settings.h>
#include <SettingsMenu.h>
#include <StatsUI.h>

#include <raymath.h>

#include <algorithm>
#include <cmath>

namespace Nawia::UI
{

    void UIHandler::triggerLocationBanner()
    {
        _location_banner_timer = 5.0f;
        if (_level_manager)
            _last_location_name = _level_manager->getCurrentLocationName();
    }

    void UIHandler::onLevelLoaded()
    {
            _location_banner_timer = 6.0f; // Wydłużone do 6 s.
        if (_level_manager)
            _last_location_name = _level_manager->getCurrentLocationName();
        _ignore_next_dt = true;
    }

    namespace
    {
        /**
         * Helper to calculate layout for vertical stacks of buttons.
         */
        std::vector<Rectangle> getVerticalMenuLayout(int button_count, bool centered = false)
        {
            const float screen_width = static_cast<float>(GetScreenWidth());
            const float screen_height = static_cast<float>(GetScreenHeight());
            
            const float scaled_width = Core::GlobalScaling::scaled(BUTTON_WIDTH);
            const float scaled_height = Core::GlobalScaling::scaled(BUTTON_HEIGHT);
            const float scaled_spacing = Core::GlobalScaling::scaled(BUTTON_SPACING);
            
            const float start_x = centered ? (screen_width - scaled_width) / 2.0f : screen_width * MENU_SIDE_X_PCT;
            
            const float total_menu_height = button_count * scaled_height + (button_count - 1) * scaled_spacing;
            const float start_y = centered 
                ? (screen_height - total_menu_height) / 2.0f + Core::GlobalScaling::scaled(40.0f) 
                : screen_height * MENU_START_Y_PCT;

            std::vector<Rectangle> button_rectangles;
            for (int i = 0; i < button_count; ++i)
            {
                const float current_y = start_y + i * (scaled_height + scaled_spacing);
                button_rectangles.push_back({ start_x, current_y, scaled_width, scaled_height });
            }

            return button_rectangles;
        }

        void drawParticlesFx(float width, float height, float time)
        {
        // Warstwy dymu.
            for (int i = 0; i < SMOKE_LAYER_COUNT; ++i)
            {
                const float seed = static_cast<float>(i) * 11.73f + 3.1f;
                const float travel = fract(hash01(seed) + time * (0.012f + hash01(seed + 2.0f) * 0.016f));
                
                const float pos_x = width * (0.05f + hash01(seed + 1.0f) * 0.90f) + std::sin(time * (0.22f + hash01(seed + 4.0f) * 0.18f) + seed) * width * 0.06f;
                const float pos_y = height * (1.12f - travel * 1.24f);
                
                const float radius = Core::GlobalScaling::scaled(110.0f + hash01(seed + 5.0f) * 150.0f);
                const float alpha = (0.35f + (1.0f - travel) * 0.65f) * (0.035f + hash01(seed + 6.0f) * 0.07f);
                
                DrawCircleGradient(static_cast<int>(pos_x), static_cast<int>(pos_y), radius, withAlpha(LIGHTGRAY, alpha), withAlpha(DARKGRAY, 0.0f));
            }

        // Cząsteczki ognia.
            for (int i = 0; i < FIRE_PARTICLE_COUNT; ++i)
            {
                const float seed = static_cast<float>(i) * 17.13f + 8.0f;
                const float cycle = fract(hash01(seed) + time * (0.10f + hash01(seed + 1.0f) * 0.22f));
                const float rise = 1.0f - cycle;
                
                const float pos_x = width * (0.03f + hash01(seed + 2.0f) * 0.94f) + std::sin(time * (1.0f + hash01(seed + 3.0f) * 1.5f) + seed) * width * (0.01f + hash01(seed + 9.0f) * 0.02f);
                const float pos_y = height * (1.04f - rise * 1.18f);
                
                const float radius = Core::GlobalScaling::scaled(1.5f + hash01(seed + 7.0f) * hash01(seed + 7.0f) * 12.0f) * (0.45f + rise * 0.95f);
                const float alpha = (0.10f + rise * 0.50f) * (0.55f + hash01(seed + 6.0f) * 0.45f);
                
                DrawCircleGradient(static_cast<int>(pos_x), static_cast<int>(pos_y), radius, withAlpha(COLOR_GOLDEN_TEXT, alpha), withAlpha(COLOR_SLAVIC_ORANGE, alpha * 0.35f));
            }
        }
    }

    UIHandler::UIHandler() : _player(nullptr), _entity_manager(nullptr) {}

    UIHandler::~UIHandler() { UnloadFont(_font); }

    void UIHandler::initialize(const std::shared_ptr<Entity::Player>& player, Core::EntityManager* entity_manager, Core::ResourceManager& resource_manager, Game::QuestManager* quest_manager, const Core::Settings* settings)
    {
        _player = player;
        _entity_manager = entity_manager;
        _quest_manager = quest_manager;
        _settings = settings;

        _font = LoadFontEx("../assets/fonts/slavic_font.ttf", Core::GlobalScaling::scaledInt(300), nullptr, 0);
        GenTextureMipmaps(&_font.texture);
        SetTextureFilter(_font.texture, TEXTURE_FILTER_TRILINEAR);

        _main_menu_background = resource_manager.getTexture("../assets/textures/main_menu.png");
        
        _inventory_ui = std::make_unique<InventoryUI>();
        _inventory_ui->loadResources(resource_manager);
        
        _chest_ui = std::make_unique<ChestUI>();
        _stats_ui = std::make_unique<StatsUI>(_player);
        _quest_ui = std::make_unique<QuestUI>();
        
        _previous_hp = _player ? _player->getHP() : 0;
        _visual_hp_percent = 1.0f;
        _visual_exp_percent = 0.0f;
    }

    void UIHandler::updateHoverTimers(float delta_time, const std::vector<Rectangle>& button_rectangles)
    {
        if (_hover_timers.size() != button_rectangles.size())
            _hover_timers.assign(button_rectangles.size(), 0.0f);
        
        const Vector2 mouse_position = GetMousePosition();
        for (size_t i = 0; i < button_rectangles.size(); ++i)
        {
            if (CheckCollisionPointRec(mouse_position, button_rectangles[i]))
                _hover_timers[i] = std::min(1.0f, _hover_timers[i] + delta_time * 6.0f);
            else
                _hover_timers[i] = std::max(0.0f, _hover_timers[i] - delta_time * 4.0f);
        }
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

        if (_is_authors_open)
        {
            const float screen_width = static_cast<float>(GetScreenWidth());
            const float screen_height = static_cast<float>(GetScreenHeight());
            const float button_width = Core::GlobalScaling::scaled(BUTTON_WIDTH);
            const float button_height = Core::GlobalScaling::scaled(BUTTON_HEIGHT);
            const float bottom_offset = Core::GlobalScaling::scaled(BACK_BUTTON_BOTTOM_OFFSET);
            
            updateHoverTimers(delta_time, {{ (screen_width - button_width) / 2.0f, screen_height - bottom_offset, button_width, button_height }});
        }
        else if (!_settings_menu && !_level_select_menu)
            updateHoverTimers(delta_time, getVerticalMenuLayout(4));

        for (auto iterator = _notifications.begin(); iterator != _notifications.end();)
        {
            iterator->timer -= delta_time;
            if (iterator->timer <= 0.0f)
                iterator = _notifications.erase(iterator);
            else
                ++iterator;
        }
        
        // Ograniczamy delta_time animacji UI, żeby skoki klatek podczas ładowania nie przeskakiwały stanów.
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

        // gui skrzyni na wysokosci gui eq
        // dialogi padding
        
        if (_location_banner_timer > 0.0f)
            _location_banner_timer -= effective_delta_time;
    }

    void UIHandler::draw_menu_buttons_stack(const std::vector<MenuButtonDef>& buttons, const std::vector<Rectangle>& rectangles) const
    {
        for (size_t i = 0; i < buttons.size(); ++i)
        {
            const float hover_progress = (i < _hover_timers.size()) ? _hover_timers[i] : 0.0f;
            drawMenuButton(rectangles[i], buttons[i].label, hover_progress);
        }
    }

    int UIHandler::get_clicked_button_index(const std::vector<Rectangle>& rectangles) const
    {
        if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            return -1;
            
        const Vector2 mouse_position = GetMousePosition();
        for (size_t i = 0; i < rectangles.size(); ++i)
        {
            if (CheckCollisionPointRec(mouse_position, rectangles[i]))
                return static_cast<int>(i);
        }
        return -1;
    }

    void UIHandler::renderVerticalMenu(const char* title, const std::vector<MenuButtonDef>& buttons, bool centered) const
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float font_spacing = Core::GlobalScaling::scaled(2.0f);
        
        const float title_font_size = Core::GlobalScaling::scaled(centered ? FONT_SIZE_TITLE : FONT_SIZE_MAIN_TITLE);
        const Vector2 title_size = MeasureTextEx(_font, title, title_font_size, font_spacing);
        
        const float button_width = Core::GlobalScaling::scaled(BUTTON_WIDTH);
        
        const Vector2 title_position = centered 
            ? Vector2{(screen_width - title_size.x) / 2.0f, Core::GlobalScaling::scaled(80.0f)} 
            : Vector2{(screen_width * MENU_SIDE_X_PCT) + (button_width - title_size.x) / 2.0f, screen_height * 0.22f + std::sin(static_cast<float>(GetTime()) * 0.8f) * Core::GlobalScaling::scaled(4.0f)};
            
        if (!centered)
        {
            DrawTextEx(_font, title, { title_position.x + 6, title_position.y + 6 }, title_font_size, font_spacing, withAlpha(BLACK, 0.8f));
            DrawTextEx(_font, title, title_position, title_font_size, font_spacing, WHITE);
        }
        else
            DrawTextEx(_font, title, title_position, title_font_size, font_spacing, COLOR_ACCENT);

        const auto button_rectangles = getVerticalMenuLayout(static_cast<int>(buttons.size()), centered);
        draw_menu_buttons_stack(buttons, button_rectangles);
    }

    void UIHandler::renderMainMenu() const
    {
        if (_is_authors_open)
        {
            renderAuthorsMenu();
            return;
        }
        drawSharedMenuBackground();
        renderVerticalMenu("Nawia", { 
            {LABEL_PLAY, MenuAction::Play}, 
            {LABEL_SETTINGS, MenuAction::Settings}, 
            {LABEL_AUTHORS, MenuAction::Authors}, 
            {LABEL_EXIT, MenuAction::Exit} 
        });
    }

    void UIHandler::renderAuthorsMenu() const
    {
        drawSharedMenuBackground();
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float subtitle_font_size = Core::GlobalScaling::scaled(FONT_SIZE_SUBTITLE);
        
        const char* author_names[] = { AUTHOR_NAME_1, AUTHOR_NAME_2, AUTHOR_NAME_3, AUTHOR_NAME_4 };
        float current_y = screen_height * 0.35f;
        
        for (const auto* name : author_names)
        {
            const Vector2 name_size = MeasureTextEx(_font, name, subtitle_font_size, 2.0f);
            DrawTextEx(_font, name, {(screen_width - name_size.x) / 2.0f, current_y}, subtitle_font_size, 2.0f, WHITE);
            current_y += subtitle_font_size + Core::GlobalScaling::scaled(20.0f);
        }
        
        const float button_width = Core::GlobalScaling::scaled(BUTTON_WIDTH);
        const float button_height = Core::GlobalScaling::scaled(BUTTON_HEIGHT);
        const float bottom_offset = Core::GlobalScaling::scaled(BACK_BUTTON_BOTTOM_OFFSET);
        
        const float hover_progress = _hover_timers.empty() ? 0.0f : _hover_timers[0];
        drawMenuButton({ (screen_width - button_width) / 2.0f, screen_height - bottom_offset, button_width, button_height }, LABEL_BACK, hover_progress);
    }

    MenuAction UIHandler::handleMenuInput()
    {
        if (_is_authors_open)
        {
            const float screen_width = static_cast<float>(GetScreenWidth());
            const float screen_height = static_cast<float>(GetScreenHeight());
            const float button_width = Core::GlobalScaling::scaled(BUTTON_WIDTH);
            const float button_height = Core::GlobalScaling::scaled(BUTTON_HEIGHT);
            const float bottom_offset = Core::GlobalScaling::scaled(BACK_BUTTON_BOTTOM_OFFSET);
            
            const Rectangle back_rectangle = { (screen_width - button_width) / 2.0f, screen_height - bottom_offset, button_width, button_height };
            if (get_clicked_button_index({back_rectangle}) == 0)
            {
                _is_authors_open = false;
                return MenuAction::None;
            }
        }
        else
        {
            const auto button_rectangles = getVerticalMenuLayout(4, false);
            const int clicked_index = get_clicked_button_index(button_rectangles);
            
            if (clicked_index == 0) return MenuAction::Play;
            if (clicked_index == 1) return MenuAction::Settings;
            if (clicked_index == 2) return MenuAction::Authors;
            if (clicked_index == 3) return MenuAction::Exit;
        }
        
        if (IsKeyPressed(KEY_ESCAPE) && _is_authors_open)
            _is_authors_open = false;

        return MenuAction::None;
    }

    void UIHandler::drawMenuButton(const Rectangle& rectangle, const char* text, float hover_progress) const
    {
        DrawRectangleRec(rectangle, withAlpha(COLOR_PANEL_BG, 0.45f + hover_progress * 0.35f));
        
        const Color border_color = LerpColor(withAlpha(WHITE, 0.35f), withAlpha(COLOR_ACCENT, 0.9f), hover_progress);
        DrawRectangleLinesEx(rectangle, Core::GlobalScaling::scaled(2.0f), border_color);
        
        if (hover_progress > 0.01f)
            DrawRectangleGradientV(static_cast<int>(rectangle.x), static_cast<int>(rectangle.y), static_cast<int>(rectangle.width), static_cast<int>(rectangle.height), withAlpha(WHITE, 0.06f * hover_progress), withAlpha(WHITE, 0.0f));
        
        const float button_font_size = Core::GlobalScaling::scaled(FONT_SIZE_BUTTON);
        const Vector2 text_size = MeasureTextEx(_font, text, button_font_size, 2.0f);
        const Vector2 text_position = { 
            rectangle.x + (rectangle.width - text_size.x) / 2.0f + hover_progress * Core::GlobalScaling::scaled(12.0f), 
            rectangle.y + (rectangle.height - text_size.y) / 2.0f 
        };
        
        DrawTextEx(_font, text, { text_position.x + 2, text_position.y + 2 }, button_font_size, 2.0f, withAlpha(BLACK, 0.5f));
        DrawTextEx(_font, text, text_position, button_font_size, 2.0f, LerpColor(withAlpha(WHITE, 0.95f), COLOR_GOLDEN_TEXT, hover_progress));
    }

    void UIHandler::drawSharedMenuBackground() const
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float current_time = static_cast<float>(GetTime());
        
        if (_main_menu_background)
        {
            float source_width = static_cast<float>(_main_menu_background->width);
            float source_height = static_cast<float>(_main_menu_background->height);
            const float screen_aspect_ratio = screen_width / screen_height;
            const float texture_aspect_ratio = source_width / source_height;
            
            if (texture_aspect_ratio > screen_aspect_ratio)
                source_width = source_height * screen_aspect_ratio;
            else
                source_height = source_width / screen_aspect_ratio;
            
            const float zoom_factor = 0.11f + 0.02f * std::sin(current_time * 0.23f);
            source_width *= (1.0f - zoom_factor);
            source_height *= (1.0f - zoom_factor);
            
            const float offset_x = std::max(0.0f, (_main_menu_background->width - source_width) * 0.5f) * std::sin(current_time * 0.12f + 0.8f);
            const float offset_y = std::max(0.0f, (_main_menu_background->height - source_height) * 0.5f) * std::cos(current_time * 0.09f - 0.35f);
            
            const Rectangle source_rectangle = { (_main_menu_background->width - source_width) * 0.5f + offset_x, (_main_menu_background->height - source_height) * 0.5f + offset_y, source_width, source_height };
            DrawTexturePro(*_main_menu_background, source_rectangle, { 0, 0, screen_width, screen_height }, { 0, 0 }, 0.0f, WHITE);
        }
        else
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f));
        
        DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), withAlpha({ 30, 14, 10, 255 }, 0.10f), withAlpha({ 5, 5, 8, 255 }, 0.48f));
        drawParticlesFx(screen_width, screen_height, current_time);
        DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), withAlpha(COLOR_ACCENT, 0.02f), withAlpha(BLACK, 0.12f));
    }

    void UIHandler::render(const Core::GameCamera& camera)
    {
        if (!_player || !_entity_manager)
            return;
            
        renderPlayerExperienceBar();
        renderPlayerHealthBar();
        renderPlayerAbilityBar();
        renderCombatEntityHealthBars(camera);
        renderLocationInfo();
        
        if (_damage_flash_timer > 0.0f)
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RED, (_damage_flash_timer / 0.3f) * 0.4f));
            
        if (_settings && _settings->show_fps)
        {
            const char* fps_text = TextFormat("%d FPS", GetFPS());
            const float font_size = Core::GlobalScaling::scaled(18.0f);
            const float margin = Core::GlobalScaling::scaled(20.0f);
            const Vector2 text_size = MeasureTextEx(_font, fps_text, font_size, 1.0f);
            DrawTextEx(_font, fps_text, { static_cast<float>(GetScreenWidth()) - text_size.x - margin, margin }, font_size, 1.0f, WHITE);
        }

        // Powiadomienia.
        float current_notify_y = 10.0f;
        for (const auto& notification : _notifications)
        {
            const Vector2 text_size = MeasureTextEx(_font, notification.text.c_str(), Core::GlobalScaling::scaled(FONT_SIZE_BUTTON), 1.0f);
            const float pos_x = GetScreenWidth() - text_size.x - 20.0f;
            
            DrawRectangle(static_cast<int>(pos_x - 5), static_cast<int>(current_notify_y - 5), static_cast<int>(text_size.x + 10), static_cast<int>(text_size.y + 10), Fade(BLACK, 0.7f));
            DrawRectangleLines(static_cast<int>(pos_x - 5), static_cast<int>(current_notify_y - 5), static_cast<int>(text_size.x + 10), static_cast<int>(text_size.y + 10), WHITE);
            DrawTextEx(_font, notification.text.c_str(), { pos_x, current_notify_y }, Core::GlobalScaling::scaled(FONT_SIZE_BUTTON), 1.0f, WHITE);
            
            current_notify_y += text_size.y + 20.0f;
        }
        
        _dialogueUI.render(_font);
        
        if (_is_inventory_open)
        {
            _inventory_ui->render(_font, *_player);
            if (_stats_ui)
                _stats_ui->render(Core::GlobalScaling::scaled(50.0f), static_cast<float>(GetScreenHeight()) - Core::GlobalScaling::scaled(220.0f + 50.0f), _font);
            if (_current_container)
                _chest_ui->render(*_current_container->getInventory(), _font);
        }
        
        if (_is_quest_ui_open)
            _quest_ui->render(_font, _quest_manager);
    }

    void UIHandler::renderPauseMenu() const
    {
        drawSharedMenuBackground();
        renderVerticalMenu(LABEL_PAUSE, { 
            {LABEL_CONTINUE, MenuAction::Play}, 
            {LABEL_SETTINGS, MenuAction::Settings}, 
            {LABEL_EXIT, MenuAction::Exit} 
        }, true);
    }

    MenuAction UIHandler::handlePauseMenuInput()
    {
        const int clicked_index = get_clicked_button_index(getVerticalMenuLayout(3, true));
        
        if (clicked_index == 0) return MenuAction::Play;
        if (clicked_index == 1) return MenuAction::Settings;
        if (clicked_index == 2) return MenuAction::Exit;
        
        return MenuAction::None;
    }

    void UIHandler::renderGameOverScreen() const
    {
        drawSharedMenuBackground();
        renderVerticalMenu(LABEL_GAME_OVER, { 
            {LABEL_RESPAWN, MenuAction::Respawn}, 
            {LABEL_EXIT, MenuAction::Exit} 
        }, true);
    }

    MenuAction UIHandler::handleGameOverInput()
    {
        const int clicked_index = get_clicked_button_index(getVerticalMenuLayout(2, true));
        
        if (clicked_index == 0) return MenuAction::Respawn;
        if (clicked_index == 1) return MenuAction::Exit;
        
        return MenuAction::None;
    }

    void UIHandler::drawBar(float x, float y, float width, float height, float percentage, Color foreground_color, Color background_color) const
    {
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height), background_color);
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width * percentage), static_cast<int>(height), foreground_color);
    }

    void UIHandler::showNotification(const std::string& text, float duration)
    {
        _notifications.push_back({ text, duration, duration });
    }

    void UIHandler::handleInput()
    {
        if (_dialogueUI.isOpen())
        {
            _dialogueUI.handleInput();
            return;
        }
        
        if (IsKeyPressed(KEY_I) || IsKeyPressed(KEY_TAB))
        {
            if (_current_container) closeContainer();
            toggleInventory();
        }
        
        if (IsKeyPressed(KEY_P)) toggleQuestUI();
        if (_is_quest_ui_open) _quest_ui->handleInput();
        
        if (_is_inventory_open)
        {
            const int backpack_slot = _inventory_ui->handleInput();
            if (backpack_slot != -1)
            {
                _player->equipItemFromBackpack(backpack_slot);
                return;
            }
            
            const auto equipment_slot = _inventory_ui->getClickedEquipmentSlot();
            if (equipment_slot != Item::EquipmentSlot::None) _player->unequipItem(equipment_slot);
            
            if (_current_container)
            {
                const int container_slot = _chest_ui->handleInput();
                if (container_slot != -1)
                {
                    auto& container_inventory = *_current_container->getInventory();
                    const auto item = container_inventory.getItem(container_slot);
                    if (item && _player->getBackpack().addItem(item))
                    {
                        container_inventory.removeItem(container_slot);
                        if (_quest_manager) _quest_manager->notifyItemCollected(item->getId());
                    }
                }
            }
        }
    }

    void UIHandler::drawOrb(float center_x, float center_y, float radius, float target_percent, float ghost_percent, float wave_speed, Color fill_bright, Color fill_dark, Color bg_color, const char* text) const
    {
        // Zewnętrzna poświata.
        DrawCircleGradient(static_cast<int>(center_x), static_cast<int>(center_y), radius + Core::GlobalScaling::scaled(6.0f), withAlpha(fill_dark, 0.25f * target_percent), withAlpha(BLACK, 0.0f));
        
        // Tło kuli.
        DrawCircleV({ center_x, center_y }, radius, bg_color);
        
        // Cień wypełnienia pokazujący historię obrażeń.
        if (ghost_percent > target_percent)
        {
            const float ghost_fill_height = radius * 2.0f * ghost_percent;
            const float ghost_clip_y = static_cast<int>(center_y + radius - ghost_fill_height);
            BeginScissorMode(static_cast<int>(center_x - radius), static_cast<int>(ghost_clip_y), static_cast<int>(radius * 2.0f), static_cast<int>(ghost_fill_height));
            DrawCircleV({ center_x, center_y }, radius - 1.0f, withAlpha(COLOR_HEALTH_GHOST, 0.3f));
            EndScissorMode();
        }
        
        // Wypełnienie od dołu z przycinaniem scissor.
        const float fill_height = radius * 2.0f * target_percent;
        const float clip_y = center_y + radius - fill_height;
        BeginScissorMode(static_cast<int>(center_x - radius), static_cast<int>(clip_y), static_cast<int>(radius * 2.0f), static_cast<int>(fill_height));
        DrawCircleGradient(static_cast<int>(center_x), static_cast<int>(center_y), radius - 1.0f, fill_bright, fill_dark);
        EndScissorMode();
        
        // Falujący refleks na powierzchni płynu.
        const float surface_y = clip_y;
        const float wave_offset = std::sin(static_cast<float>(GetTime()) * wave_speed) * Core::GlobalScaling::scaled(1.5f);
        if (target_percent > 0.02f && target_percent < 0.98f)
        {
        // Liczymy szerokość koła na wysokości linii wypełnienia.
            const float dy = (surface_y + wave_offset) - center_y;
            const float half_width = std::sqrt(std::max(0.0f, radius * radius - dy * dy));
            DrawLineEx({ center_x - half_width, surface_y + wave_offset }, { center_x + half_width, surface_y + wave_offset }, Core::GlobalScaling::scaled(1.5f), withAlpha(WHITE, 0.25f));
        }
        
        // Szklany refleks w lewym górnym rogu.
        DrawCircleGradient(static_cast<int>(center_x - radius * 0.3f), static_cast<int>(center_y - radius * 0.3f), radius * 0.55f, withAlpha(WHITE, 0.12f), withAlpha(WHITE, 0.0f));
        
        // Pierścień obramowania.
        DrawCircleLinesV({ center_x, center_y }, radius, withAlpha(COLOR_ACCENT, 0.6f));
        DrawCircleLinesV({ center_x, center_y }, radius + 1.0f, withAlpha(BLACK, 0.4f));
        
        // Tekst.
        const float font_size = Core::GlobalScaling::scaled(18.0f);
        const Vector2 text_size = MeasureTextEx(_font, text, font_size, 1.0f);
        DrawTextEx(_font, text, { center_x - text_size.x / 2.0f + 1.0f, center_y - text_size.y / 2.0f + 1.0f }, font_size, 1.0f, withAlpha(BLACK, 0.6f));
        DrawTextEx(_font, text, { center_x - text_size.x / 2.0f, center_y - text_size.y / 2.0f }, font_size, 1.0f, WHITE);
    }

    void UIHandler::renderPlayerHealthBar() const
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float icon_size = Core::GlobalScaling::scaled(ABILITY_ICON_SIZE);
        const float icon_spacing = Core::GlobalScaling::scaled(ABILITY_SPACING);
        const float total_ability_width = (icon_size * 4) + (icon_spacing * 3);
        const float ability_start_x = (screen_width - total_ability_width) / 2.0f;
        const float ability_center_y = screen_height - Core::GlobalScaling::scaled(140.0f) + icon_size / 2.0f;
        
        const float orb_radius = Core::GlobalScaling::scaled(38.0f);
        const float orb_gap = Core::GlobalScaling::scaled(18.0f);
        const float orb_center_x = ability_start_x - orb_gap - orb_radius;
        const float orb_center_y = ability_center_y;
        
        const int display_hp = _player->isDying() ? 0 : _player->getHP();
        const float target_hp = std::clamp(static_cast<float>(display_hp) / _player->getMaxHP(), 0.0f, 1.0f);
        
        // Kolory wypełnienia kuli.
        const Color orb_fill_dark = { 120, 10, 10, 255 };
        const Color orb_fill_bright = { 200, 30, 30, 255 };
        const Color orb_bg = { 20, 12, 12, 240 };
        
        const char* health_text = TextFormat("%d HP", display_hp);
        drawOrb(orb_center_x, orb_center_y, orb_radius, target_hp, _visual_hp_percent, 2.5f, orb_fill_bright, orb_fill_dark, orb_bg, health_text);
    }

    void UIHandler::renderCombatEntityHealthBars(const Core::GameCamera& camera) const
    {
        for (const auto& entity : _entity_manager->getEntities())
        {
            if (!entity->isDormant() && (entity->getFaction() == Entity::Faction::Enemy || entity->getFaction() == Entity::Faction::Ally) && entity->getHP() < entity->getMaxHP() && entity->getHP() > 0)
            {
                const Vector2 screen_position = entity->getScreenPosition(camera.get());
                const float bar_width = 40.0f;
                const float bar_height = 6.0f;
                const float pos_x = screen_position.x - bar_width / 2.0f;
                const float pos_y = screen_position.y - 60.0f * Core::GlobalScaling::getScale();
                
                const float hp_percentage = std::clamp(static_cast<float>(entity->getHP()) / entity->getMaxHP(), 0.0f, 1.0f);
                drawBar(pos_x, pos_y, bar_width, bar_height, hp_percentage, RED, DARKGRAY);
                DrawRectangleLinesEx({ pos_x, pos_y, bar_width, bar_height }, 1.0f, BLACK);
            }
        }
    }

    void UIHandler::renderPlayerAbilityBar() const
    {
        const auto& abilities = _player->getAbilities();
        const float icon_size = Core::GlobalScaling::scaled(ABILITY_ICON_SIZE);
        const float icon_spacing = Core::GlobalScaling::scaled(ABILITY_SPACING);
        const float total_width = (icon_size * 4) + (icon_spacing * 3);
        const float start_x = (static_cast<float>(GetScreenWidth()) - total_width) / 2.0f;
        const float start_y = static_cast<float>(GetScreenHeight()) - Core::GlobalScaling::scaled(140.0f);
        
        for (int i = 0; i < 4; ++i)
        {
            const float pos_x = start_x + (icon_size + icon_spacing) * i;
            const Rectangle icon_rectangle = { pos_x, start_y, icon_size, icon_size };
            
            DrawRectangleRec(icon_rectangle, withAlpha(BLACK, 0.5f));
            DrawRectangleLinesEx(icon_rectangle, 2.0f, DARKGRAY);
            
            if (static_cast<size_t>(i) >= abilities.size())
                continue;
            
            const auto& ability = abilities[i];
            if (const auto icon_texture = ability->getIcon())
            {
                const Rectangle source_rectangle = { 0, 0, static_cast<float>(icon_texture->width), static_cast<float>(icon_texture->height) };
                DrawTexturePro(*icon_texture, source_rectangle, icon_rectangle, { 0, 0 }, 0.0f, WHITE);
            }
            
            if (!ability->isReady())
            {
                const float cooldown_ratio = ability->getCooldownTimer() / ability->getStats().cooldown;
                DrawRectangle(static_cast<int>(pos_x), static_cast<int>(start_y), static_cast<int>(icon_size), static_cast<int>(icon_size * cooldown_ratio), withAlpha(GRAY, 0.8f));
                
                const char* cd_text = TextFormat("%.1f", ability->getCooldownTimer());
                const Vector2 text_size = MeasureTextEx(_font, cd_text, 20.0f, 1.0f);
                DrawTextEx(_font, cd_text, { pos_x + (icon_size - text_size.x) / 2.0f, start_y + (icon_size - text_size.y) / 2.0f }, 20.0f, 1.0f, WHITE);
            }
        }
    }

    void UIHandler::renderPlayerExperienceBar() const
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float icon_size = Core::GlobalScaling::scaled(ABILITY_ICON_SIZE);
        const float icon_spacing = Core::GlobalScaling::scaled(ABILITY_SPACING);
        const float total_ability_width = (icon_size * 4) + (icon_spacing * 3);
        const float ability_end_x = (screen_width + total_ability_width) / 2.0f;
        const float ability_center_y = screen_height - Core::GlobalScaling::scaled(140.0f) + icon_size / 2.0f;
        
        const float orb_radius = Core::GlobalScaling::scaled(38.0f);
        const float orb_gap = Core::GlobalScaling::scaled(18.0f);
        const float orb_center_x = ability_end_x + orb_gap + orb_radius;
        const float orb_center_y = ability_center_y;
        
        // Kolory wypełnienia kuli.
        const Color orb_fill_dark = { 15, 40, 120, 255 };
        const Color orb_fill_bright = { 50, 100, 220, 255 };
        const Color orb_bg = { 10, 12, 25, 240 };
        
        const char* level_text = TextFormat("%d LVL", _player->getLevel());
        drawOrb(orb_center_x, orb_center_y, orb_radius, _visual_exp_percent, 0.0f, 2.0f, orb_fill_bright, orb_fill_dark, orb_bg, level_text);
    }

    void UIHandler::renderLocationInfo() const
    {
        if (!_level_manager || _level_manager->getCurrentLevelName().empty() || _location_banner_timer <= 0.0f)
            return;
            
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float alpha = std::clamp(_location_banner_timer > 4.0f ? (5.0f - _location_banner_timer) : (_location_banner_timer / 1.0f), 0.0f, 1.0f);
        
        const float banner_height = Core::GlobalScaling::scaled(100.0f);
        const float banner_y = Core::GlobalScaling::scaled(60.0f) * alpha; // Efekt wsunięcia.
        
        // Filmowe tło.
        DrawRectangleGradientH(0, static_cast<int>(banner_y), static_cast<int>(screen_width / 2), static_cast<int>(banner_height), withAlpha(BLACK, 0.0f), withAlpha(BLACK, 0.7f * alpha));
        DrawRectangleGradientH(static_cast<int>(screen_width / 2), static_cast<int>(banner_y), static_cast<int>(screen_width / 2), static_cast<int>(banner_height), withAlpha(BLACK, 0.7f * alpha), withAlpha(BLACK, 0.0f));
        
        const float level_font_size = Core::GlobalScaling::scaled(32.0f);
        const float location_font_size = Core::GlobalScaling::scaled(20.0f);
        
        const std::string& level_name = _level_manager->getCurrentLevelName();
        const std::string& location_name = _level_manager->getCurrentLocationName();
        
        const Vector2 level_size = MeasureTextEx(_font, level_name.c_str(), level_font_size, 2.0f);
        const Vector2 location_size = MeasureTextEx(_font, location_name.c_str(), location_font_size, 1.0f);
        
        DrawTextEx(_font, level_name.c_str(), { (screen_width - level_size.x) / 2.0f, banner_y + 20.0f }, level_font_size, 2.0f, withAlpha(COLOR_ACCENT, alpha));
        DrawTextEx(_font, location_name.c_str(), { (screen_width - location_size.x) / 2.0f, banner_y + 20.0f + level_size.y + 4.0f }, location_font_size, 1.0f, withAlpha(WHITE, alpha * 0.8f));
        
        // Linie.
        const float line_width = Core::GlobalScaling::scaled(200.0f) * alpha;
        DrawLineEx({ screen_width / 2.0f - line_width, banner_y }, { screen_width / 2.0f + line_width, banner_y }, 1.0f, withAlpha(COLOR_ACCENT, alpha * 0.5f));
        DrawLineEx({ screen_width / 2.0f - line_width, banner_y + banner_height }, { screen_width / 2.0f + line_width, banner_y + banner_height }, 1.0f, withAlpha(COLOR_ACCENT, alpha * 0.5f));
    }

    void UIHandler::renderSettingsMenu() const { if (_settings_menu) { drawSharedMenuBackground(); _settings_menu->render(*this); } }
    MenuAction UIHandler::handleSettingsInput() { if (!_settings_menu) return MenuAction::None; if (_settings_menu->handleInput()) { _settings_menu.reset(); return MenuAction::Play; } return MenuAction::None; }
    void UIHandler::openSettings(const Core::Settings& s) { _settings_menu = std::make_unique<SettingsMenu>(s); }
    bool UIHandler::wereSettingsApplied() const { return _settings_menu && _settings_menu->wasApplied(); }
    const Core::Settings& UIHandler::getAppliedSettings() const { return _settings_menu->getSettings(); }
    void UIHandler::closeSettingsMenu() { _settings_menu.reset(); }
    void UIHandler::renderLevelSelectMenu() const { if (_level_select_menu) { drawSharedMenuBackground(); _level_select_menu->render(*this); } }
    void UIHandler::openLevelSelect(const std::vector<World::LevelInfo>& l) { _level_select_menu = std::make_unique<LevelSelectMenu>(l); }
    void UIHandler::closeLevelSelect() { _level_select_menu.reset(); }
    std::string UIHandler::handleLevelSelectInput() { if (IsKeyPressed(KEY_ESCAPE)) return "BACK"; if (_level_select_menu) return _level_select_menu->handleInput(); return ""; }
    void UIHandler::openContainer(Entity::InteractiveClickable* c) { _current_container = c; _is_inventory_open = true; }
    void UIHandler::closeContainer() { _current_container = nullptr; }
    bool UIHandler::isInputBlocked() const { return _dialogueUI.isOpen() || _current_container || isMouseOverUI(); }

    bool UIHandler::isMouseOverUI() const
    {
        const Vector2 mouse_pos = GetMousePosition();

        if (_is_inventory_open)
        {
            const float inv_x = Core::GlobalScaling::scaled(50.0f);
            const float inv_y = Core::GlobalScaling::scaled(50.0f);
            const float inv_width = Core::GlobalScaling::scaled(InventoryUI::INV_WIDTH);
            const float inv_height = Core::GlobalScaling::scaled(InventoryUI::INV_HEIGHT);
            const Rectangle rect = { inv_x, inv_y, inv_width, inv_height };
            if (CheckCollisionPointRec(mouse_pos, rect))
                return true;
        }

        if (_is_quest_ui_open)
        {
            const float menu_width = Core::GlobalScaling::scaled(QuestUI::MENU_WIDTH);
            const float menu_height = Core::GlobalScaling::scaled(QuestUI::MENU_HEIGHT);
            const float screen_width = static_cast<float>(GetScreenWidth());
            const float start_x = screen_width - menu_width - Core::GlobalScaling::scaled(50.0f);
            const float start_y = Core::GlobalScaling::scaled(50.0f);
            const Rectangle rect = { start_x, start_y, menu_width, menu_height };
            if (CheckCollisionPointRec(mouse_pos, rect))
                return true;
        }

        return false;
    }

    bool UIHandler::closeOpenWindows()
    {
        bool closed_anything = false;
        
        if (_is_inventory_open) {
            _is_inventory_open = false;
            closed_anything = true;
        }
        if (_is_quest_ui_open) {
            _is_quest_ui_open = false;
            closed_anything = true;
        }
        if (_current_container) {
            closeContainer();
            closed_anything = true;
        }
        if (_dialogueUI.isOpen()) {
            closeDialogue();
            closed_anything = true;
        }
        
        return closed_anything;
    }

} // namespace Nawia::UI
