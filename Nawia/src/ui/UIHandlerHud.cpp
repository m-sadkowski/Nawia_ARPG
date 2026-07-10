#include "UIHandler.h"

#include <Ability.h>
#include <BossManager.h>
#include <Camera.h>
#include <Entity.h>
#include <EntityManager.h>
#include <EnemyInterface.h>
#include <GlobalScaling.h>
#include <LevelManager.h>
#include <Player.h>

#include <algorithm>
#include <cmath>

namespace Nawia::UI
{
    namespace
    {
        Color getBossBarColor(const float hp_percent)
        {
            if (hp_percent > 0.5f) {
                const float t = (hp_percent - 0.5f) / 0.5f;
                return {
                    static_cast<unsigned char>(255.0f * (1.0f - t)),
                    static_cast<unsigned char>(180.0f + 75.0f * t),
                    30, 255
                };
            }
            if (hp_percent > 0.2f) {
                const float t = (hp_percent - 0.2f) / 0.3f;
                return {
                    255,
                    static_cast<unsigned char>(100.0f + 80.0f * t),
                    20, 255
                };
            }
            return {180, 32, 26, 255};
        }
    }

    void UIHandler::drawBar(
        const float x,
        const float y,
        const float width,
        const float height,
        const float percentage,
        const Color foreground_color,
        const Color background_color) const
    {
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height), background_color);
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width * percentage), static_cast<int>(height), foreground_color);
    }

    void UIHandler::renderBossName(const std::string& name, const float x, const float y, const float bar_width, const float spacing) const
    {
        const float name_font_size = Core::GlobalScaling::scaled(26.0f);
        const Vector2 name_size = MeasureTextEx(_font, name.c_str(), name_font_size, spacing);
        const Vector2 name_position = {
            x + (bar_width - name_size.x) * 0.5f,
            y - name_size.y - Core::GlobalScaling::scaled(8.0f)
        };
        drawTextWithShadow(_font, name.c_str(), name_position, name_font_size, spacing, GOLD, {2.0f, 2.0f}, Fade(BLACK, 0.8f));
    }

    void UIHandler::renderBossPhaseMarkers(const Game::BossData& boss_data, const float x, const float y, const float bar_width, const float bar_height) const
    {
        for (size_t i = 1; i < boss_data.phases.size(); ++i)
        {
            const float marker_x = x + bar_width * std::clamp(boss_data.phases[i].hp_threshold, 0.0f, 1.0f);
            DrawRectangle(static_cast<int>(marker_x) - 1, static_cast<int>(y), 3, static_cast<int>(bar_height), Fade(WHITE, 0.5f));
        }
    }

    void UIHandler::renderBossFightInfo(
        const Game::BossManager* boss_manager,
        const float x,
        const float y,
        const float bar_width,
        const float bar_height,
        const float spacing) const
    {
        const auto* boss_data = boss_manager->getActiveBossData();
        const int phase_index = boss_manager->getCurrentPhaseIndex();
        const float info_y = y + bar_height + Core::GlobalScaling::scaled(5.0f);
        const float info_font_size = Core::GlobalScaling::scaled(14.0f);

        if (boss_data && phase_index >= 0 && phase_index < static_cast<int>(boss_data->phases.size()))
            DrawTextEx(_font, boss_data->phases[phase_index].name.c_str(), {x + 2.0f, info_y}, info_font_size, spacing, Fade(WHITE, 0.75f));

        const float timer = boss_manager->getFightTimer();
        const char* timer_text = TextFormat("%02d:%02d", static_cast<int>(timer) / 60, static_cast<int>(timer) % 60);
        const Vector2 timer_size = MeasureTextEx(_font, timer_text, info_font_size, spacing);
        DrawTextEx(_font, timer_text, {x + bar_width - timer_size.x - 2.0f, info_y}, info_font_size, spacing, Fade(WHITE, 0.75f));
    }

    void UIHandler::renderBossHealthBar(const Game::BossManager* boss_manager) const
    {
        if (!boss_manager || !boss_manager->isFightActive())
            return;

        const auto* boss_data = boss_manager->getActiveBossData();
        const auto boss_entity = boss_manager->getActiveBossEntity();
        if (!boss_data || !boss_entity || boss_entity->getMaxHP() <= 0)
            return;

        const float screen_width = static_cast<float>(GetScreenWidth());
        const float bar_width = screen_width * 0.55f;
        const float bar_height = Core::GlobalScaling::scaled(28.0f);
        const float x = (screen_width - bar_width) * 0.5f;
        const float y = Core::GlobalScaling::scaled(50.0f);
        const float padding = Core::GlobalScaling::scaled(12.0f);
        const float spacing = Core::GlobalScaling::scaled(1.5f);
        const float hp_percent = std::clamp(
            static_cast<float>(boss_entity->getHP()) / static_cast<float>(boss_entity->getMaxHP()),
            0.0f,
            1.0f);

        const Rectangle panel = {
            x - padding,
            y - Core::GlobalScaling::scaled(40.0f),
            bar_width + padding * 2.0f,
            bar_height + Core::GlobalScaling::scaled(64.0f)
        };
        DrawRectangleRec(panel, Fade(BLACK, 0.70f));
        DrawRectangleLinesEx(panel, Core::GlobalScaling::scaled(1.5f), Fade(GOLD, 0.45f));

        renderBossName(boss_data->name, x, y, bar_width, spacing);

        const Color bar_color = getBossBarColor(hp_percent);
        DrawRectangleRec({x, y, bar_width, bar_height}, {20, 20, 20, 220});
        DrawRectangleRec({x, y, bar_width * hp_percent, bar_height}, bar_color);
        DrawRectangleRec({x, y, bar_width * hp_percent, bar_height * 0.35f}, Fade(WHITE, 0.08f));
        renderBossPhaseMarkers(*boss_data, x, y, bar_width, bar_height);
        DrawRectangleLinesEx({x, y, bar_width, bar_height}, Core::GlobalScaling::scaled(2.5f), GOLD);

        const char* hp_text = TextFormat("%d / %d", boss_entity->getHP(), boss_entity->getMaxHP());
        const float hp_font_size = Core::GlobalScaling::scaled(16.0f);
        const Vector2 hp_size = MeasureTextEx(_font, hp_text, hp_font_size, spacing);
        const Vector2 hp_position = {
            x + (bar_width - hp_size.x) * 0.5f,
            y + (bar_height - hp_size.y) * 0.5f
        };
        drawTextWithShadow(_font, hp_text, hp_position, hp_font_size, spacing, WHITE, {1.0f, 1.0f}, Fade(BLACK, 0.6f));
        renderBossFightInfo(boss_manager, x, y, bar_width, bar_height, spacing);
    }

    void UIHandler::drawOrb(
        const float center_x,
        const float center_y,
        const float radius,
        const float target_percent,
        const float ghost_percent,
        const float wave_speed,
        const Color fill_bright,
        const Color fill_dark,
        const Color bg_color,
        const char* text,
        const std::shared_ptr<Texture2D>& frame_texture) const
    {
        DrawCircleGradient(static_cast<int>(center_x), static_cast<int>(center_y), radius + Core::GlobalScaling::scaled(6.0f), withAlpha(fill_dark, 0.25f * target_percent), withAlpha(BLACK, 0.0f));
        DrawCircleV({center_x, center_y}, radius, bg_color);

        if (ghost_percent > target_percent)
        {
            const float ghost_fill_height = radius * 2.0f * ghost_percent;
            const float ghost_clip_y = static_cast<int>(center_y + radius - ghost_fill_height);
            BeginScissorMode(static_cast<int>(center_x - radius), static_cast<int>(ghost_clip_y), static_cast<int>(radius * 2.0f), static_cast<int>(ghost_fill_height));
            DrawCircleV({center_x, center_y}, radius - 1.0f, withAlpha(COLOR_HEALTH_GHOST, 0.3f));
            EndScissorMode();
        }

        const float fill_height = radius * 2.0f * target_percent;
        const float clip_y = center_y + radius - fill_height;
        BeginScissorMode(static_cast<int>(center_x - radius), static_cast<int>(clip_y), static_cast<int>(radius * 2.0f), static_cast<int>(fill_height));
        DrawCircleGradient(static_cast<int>(center_x), static_cast<int>(center_y), radius - 1.0f, fill_bright, fill_dark);
        EndScissorMode();

        const float surface_y = clip_y;
        const float wave_offset = std::sin(static_cast<float>(GetTime()) * wave_speed) * Core::GlobalScaling::scaled(1.5f);
        if (target_percent > 0.02f && target_percent < 0.98f)
        {
            const float dy = (surface_y + wave_offset) - center_y;
            const float half_width = std::sqrt(std::max(0.0f, radius * radius - dy * dy));
            DrawLineEx({center_x - half_width, surface_y + wave_offset}, {center_x + half_width, surface_y + wave_offset}, Core::GlobalScaling::scaled(1.5f), withAlpha(WHITE, 0.25f));
        }

        DrawCircleGradient(static_cast<int>(center_x - radius * 0.3f), static_cast<int>(center_y - radius * 0.3f), radius * 0.55f, withAlpha(WHITE, 0.12f), withAlpha(WHITE, 0.0f));

        if (frame_texture && frame_texture->id > 0)
        {
            const float frame_size = radius * 2.9f;
            const Rectangle frame_destination = {
                center_x - frame_size / 2.0f,
                center_y - frame_size / 2.0f,
                frame_size,
                frame_size
            };
            DrawTexturePro(
                *frame_texture,
                {0.0f, 0.0f, static_cast<float>(frame_texture->width), static_cast<float>(frame_texture->height)},
                frame_destination,
                {0.0f, 0.0f},
                0.0f,
                WHITE);
        }
        else
        {
            DrawCircleLinesV({center_x, center_y}, radius, withAlpha(COLOR_ACCENT, 0.6f));
            DrawCircleLinesV({center_x, center_y}, radius + 1.0f, withAlpha(BLACK, 0.4f));
        }

        const float font_size = Core::GlobalScaling::scaled(18.0f);
        drawTextWithShadow(
            _font,
            text,
            centeredTextPosition(_font, text, {center_x - radius, center_y - radius, radius * 2.0f, radius * 2.0f}, font_size, 1.0f),
            font_size,
            1.0f,
            WHITE,
            {1.0f, 1.0f},
            withAlpha(BLACK, 0.6f));
    }

    void UIHandler::renderPlayerHealthBar() const
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float ability_frame_width = Core::GlobalScaling::scaled(520.0f);
        const float ability_frame_x = (screen_width - ability_frame_width) / 2.0f;
        const float ability_center_y = screen_height - Core::GlobalScaling::scaled(126.0f) + Core::GlobalScaling::scaled(55.0f);
        const float orb_radius = Core::GlobalScaling::scaled(39.0f);
        const float orb_gap = Core::GlobalScaling::scaled(12.0f);
        const float orb_center_x = ability_frame_x - orb_gap - orb_radius;
        const float orb_center_y = ability_center_y;
        const int display_hp = _player->isDying() ? 0 : _player->getHP();
        const float target_hp = std::clamp(static_cast<float>(display_hp) / _player->getMaxHP(), 0.0f, 1.0f);
        const char* health_text = TextFormat("%d HP", display_hp);
        drawOrb(orb_center_x, orb_center_y, orb_radius, target_hp, _visual_hp_percent, 2.5f, {200, 30, 30, 255}, {120, 10, 10, 255}, {20, 12, 12, 240}, health_text, _hp_orb_frame);
    }

    void UIHandler::renderPlayerStatusEffects() const
    {
        if (!_player || (!_player->isPoisoned() && !_player->isMovementRooted()))
            return;

        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float ability_frame_width = Core::GlobalScaling::scaled(520.0f);
        const float start_x = (screen_width - ability_frame_width) / 2.0f;
        float x = start_x + Core::GlobalScaling::scaled(116.0f);
        const float y = screen_height - Core::GlobalScaling::scaled(164.0f);
        const float height = Core::GlobalScaling::scaled(24.0f);
        const float gap = Core::GlobalScaling::scaled(8.0f);
        const float font_size = Core::GlobalScaling::scaled(14.0f);
        const float spacing = Core::GlobalScaling::scaled(1.0f);

        const auto draw_status = [&](const char* label, const float remaining, const Color color) {
            const char* text = TextFormat("%s %.1f", label, remaining);
            const Vector2 text_size = MeasureTextEx(_font, text, font_size, spacing);
            const float width = text_size.x + Core::GlobalScaling::scaled(18.0f);
            const Rectangle rect = {x, y, width, height};
            DrawRectangleRounded(rect, 0.18f, 6, withAlpha(BLACK, 0.68f));
            DrawRectangleRoundedLines(rect, 0.18f, 6, withAlpha(color, 0.95f));
            DrawTextEx(_font, text, {x + Core::GlobalScaling::scaled(9.0f), y + (height - text_size.y) * 0.5f}, font_size, spacing, color);
            x += width + gap;
        };

        if (_player->isPoisoned())
            draw_status("TRUCIZNA", _player->getPoisonRemaining(), {90, 220, 90, 255});
        if (_player->isMovementRooted())
            draw_status("PAJECZYNA", _player->getRootRemaining(), {220, 220, 230, 255});
    }

    void UIHandler::renderCombatEntityHealthBars(const Core::GameCamera& camera, const Game::BossManager* boss_manager) const
    {
        std::shared_ptr<Entity::EnemyInterface> boss_entity = nullptr;
        if (boss_manager && boss_manager->isFightActive())
            boss_entity = boss_manager->getActiveBossEntity();

        for (const auto& entity : _entity_manager->getEntities())
        {
            if (entity->isDormant()) continue;
            if (entity->getFaction() != Entity::Faction::Enemy && entity->getFaction() != Entity::Faction::Ally) continue;
            if (entity->getHP() >= entity->getMaxHP() || entity->getHP() <= 0) continue;
            if (boss_entity && entity.get() == boss_entity.get()) continue;

            const BoundingBox bounding_box = entity->getBoundingBox();
            const Vector3 bar_world_position = {
                (bounding_box.min.x + bounding_box.max.x) * 0.5f,
                bounding_box.max.y + 0.35f,
                (bounding_box.min.z + bounding_box.max.z) * 0.5f
            };
            const Vector2 screen_position = GetWorldToScreen(bar_world_position, camera.get());
            const float bar_width = 40.0f;
            const float bar_height = 6.0f;
            const float pos_x = screen_position.x - bar_width / 2.0f;
            const float pos_y = screen_position.y - bar_height / 2.0f;
            const float hp_percentage = std::clamp(static_cast<float>(entity->getHP()) / entity->getMaxHP(), 0.0f, 1.0f);
            drawBar(pos_x, pos_y, bar_width, bar_height, hp_percentage, RED, DARKGRAY);
            DrawRectangleLinesEx({pos_x, pos_y, bar_width, bar_height}, 1.0f, BLACK);
        }
    }

    void UIHandler::renderPlayerAbilityBar() const
    {
        const auto& abilities = _player->getAbilities();
        const float frame_width = Core::GlobalScaling::scaled(520.0f);
        const float frame_height = Core::GlobalScaling::scaled(111.0f);
        const float frame_x = (static_cast<float>(GetScreenWidth()) - frame_width) / 2.0f;
        const float frame_y = static_cast<float>(GetScreenHeight()) - Core::GlobalScaling::scaled(126.0f);
        const float icon_size = frame_height * 0.55f;
        const float slot_center_ratios[] = {0.247f, 0.370f, 0.641f, 0.764f};
        const char* slot_keys[] = {"Q", "W", "E", "R"};
        const float slot_center_y = frame_y + frame_height * 0.53f;
        const Rectangle food_rectangle = {
            frame_x + frame_width * 0.500f - icon_size / 2.0f,
            slot_center_y - icon_size / 2.0f,
            icon_size,
            icon_size
        };

        if (_food_icon && _food_icon->id > 0) {
            DrawTexturePro(*_food_icon, {0.0f, 0.0f, static_cast<float>(_food_icon->width), static_cast<float>(_food_icon->height)}, food_rectangle, {0.0f, 0.0f}, 0.0f, WHITE);
        } else {
            DrawRectangleRec(food_rectangle, withAlpha(BROWN, 0.75f));
        }

        for (int i = 0; i < 4; ++i)
        {
            const float pos_x = frame_x + frame_width * slot_center_ratios[i] - icon_size / 2.0f - Core::GlobalScaling::scaled(4.0f);
            const float start_y = slot_center_y - icon_size / 2.0f;
            const Rectangle icon_rectangle = {pos_x, start_y, icon_size, icon_size};
            if (!_ability_bar_frame || _ability_bar_frame->id <= 0)
                DrawRectangleRec(icon_rectangle, withAlpha(BLACK, 0.5f));

            const auto draw_empty_slot_icon = [&]() {
                if (_empty_ability_icon && _empty_ability_icon->id > 0)
                    DrawTexturePro(*_empty_ability_icon, {0.0f, 0.0f, static_cast<float>(_empty_ability_icon->width), static_cast<float>(_empty_ability_icon->height)}, icon_rectangle, {0.0f, 0.0f}, 0.0f, Fade(WHITE, 0.72f));
            };

            if (static_cast<size_t>(i) >= abilities.size() || !abilities[i]) {
                draw_empty_slot_icon();
                continue;
            }

            const auto& ability = abilities[i];
            if (const auto icon_texture = ability->getIcon())
                DrawTexturePro(*icon_texture, {0, 0, static_cast<float>(icon_texture->width), static_cast<float>(icon_texture->height)}, icon_rectangle, {0, 0}, 0.0f, WHITE);

            if (!ability->isReady())
            {
                DrawRectangle(static_cast<int>(pos_x), static_cast<int>(start_y), static_cast<int>(icon_size), static_cast<int>(icon_size * ability->getCooldownRatio()), withAlpha(GRAY, 0.8f));
                const char* cd_text = TextFormat("%.1f", ability->getCooldownTimer());
                const float cooldown_font_size = Core::GlobalScaling::scaled(18.0f);
                const Vector2 text_size = MeasureTextEx(_font, cd_text, cooldown_font_size, 1.0f);
                DrawTextEx(_font, cd_text, {pos_x + (icon_size - text_size.x) / 2.0f, start_y + (icon_size - text_size.y) / 2.0f}, cooldown_font_size, 1.0f, WHITE);
            }
        }

        if (_ability_bar_frame && _ability_bar_frame->id > 0)
        {
            DrawTexturePro(*_ability_bar_frame, {0.0f, 0.0f, static_cast<float>(_ability_bar_frame->width), static_cast<float>(_ability_bar_frame->height)}, {frame_x, frame_y, frame_width, frame_height}, {0.0f, 0.0f}, 0.0f, WHITE);
        }
        else
        {
            for (int i = 0; i < 4; ++i)
            {
                const float pos_x = frame_x + frame_width * slot_center_ratios[i] - icon_size / 2.0f - Core::GlobalScaling::scaled(4.0f);
                const float start_y = slot_center_y - icon_size / 2.0f;
                DrawRectangleLinesEx({pos_x, start_y, icon_size, icon_size}, 2.0f, DARKGRAY);
            }
        }

        const auto draw_key_label = [&](const char* key_text, const Rectangle& icon_rect) {
            const float font_size = Core::GlobalScaling::scaled(17.0f);
            const Vector2 text_size = MeasureTextEx(_font, key_text, font_size, 1.0f);
            const Vector2 text_pos = {
                icon_rect.x + (icon_rect.width - text_size.x) * 0.5f,
                icon_rect.y - Core::GlobalScaling::scaled(20.0f)
            };
            drawTextWithShadow(_font, key_text, text_pos, font_size, 1.0f, COLOR_GOLDEN_TEXT, {1.0f, 1.0f}, Fade(BLACK, 0.85f));
        };

        for (int i = 0; i < 4; ++i)
        {
            const float pos_x = frame_x + frame_width * slot_center_ratios[i] - icon_size / 2.0f - Core::GlobalScaling::scaled(4.0f);
            const float start_y = slot_center_y - icon_size / 2.0f;
            draw_key_label(slot_keys[i], {pos_x, start_y, icon_size, icon_size});
        }

        draw_key_label("1", food_rectangle);

        const char* food_count_text = TextFormat("%d", _player->getFoodCount());
        const float count_font_size = Core::GlobalScaling::scaled(18.0f);
        const Vector2 count_size = MeasureTextEx(_font, food_count_text, count_font_size, 1.0f);
        const Vector2 count_pos = {
            food_rectangle.x + (food_rectangle.width - count_size.x) * 0.5f,
            food_rectangle.y + food_rectangle.height - Core::GlobalScaling::scaled(2.0f)
        };
        drawTextWithShadow(_font, food_count_text, count_pos, count_font_size, 1.0f, WHITE, {1.0f, 1.0f}, Fade(BLACK, 0.9f));
    }

    void UIHandler::renderPlayerExperienceBar() const
    {
        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());
        const float ability_frame_width = Core::GlobalScaling::scaled(520.0f);
        const float ability_frame_x = (screen_width - ability_frame_width) / 2.0f;
        const float ability_center_y = screen_height - Core::GlobalScaling::scaled(126.0f) + Core::GlobalScaling::scaled(55.0f);
        const float orb_radius = Core::GlobalScaling::scaled(39.0f);
        const float orb_gap = Core::GlobalScaling::scaled(12.0f);
        const float orb_center_x = ability_frame_x + ability_frame_width + orb_gap + orb_radius;
        const char* level_text = TextFormat("%d LVL", _player->getLevel());
        drawOrb(orb_center_x, ability_center_y, orb_radius, _visual_exp_percent, 0.0f, 2.0f, {50, 100, 220, 255}, {15, 40, 120, 255}, {10, 12, 25, 240}, level_text, _level_orb_frame);
    }

    void UIHandler::renderLocationInfo() const
    {
        if (!_level_manager || _level_manager->getCurrentLevelName().empty() || _location_banner_timer <= 0.0f)
            return;

        const float screen_width = static_cast<float>(GetScreenWidth());
        const float alpha = std::clamp(_location_banner_timer > 4.0f ? (5.0f - _location_banner_timer) : (_location_banner_timer / 1.0f), 0.0f, 1.0f);
        const float banner_height = Core::GlobalScaling::scaled(100.0f);
        const float banner_y = Core::GlobalScaling::scaled(60.0f) * alpha;
        DrawRectangleGradientH(0, static_cast<int>(banner_y), static_cast<int>(screen_width / 2), static_cast<int>(banner_height), withAlpha(BLACK, 0.0f), withAlpha(BLACK, 0.7f * alpha));
        DrawRectangleGradientH(static_cast<int>(screen_width / 2), static_cast<int>(banner_y), static_cast<int>(screen_width / 2), static_cast<int>(banner_height), withAlpha(BLACK, 0.7f * alpha), withAlpha(BLACK, 0.0f));

        const float level_font_size = Core::GlobalScaling::scaled(32.0f);
        const float location_font_size = Core::GlobalScaling::scaled(20.0f);
        const std::string& level_name = _level_manager->getCurrentLevelName();
        const std::string& location_name = _level_manager->getCurrentLocationName();
        const Vector2 level_size = MeasureTextEx(_font, level_name.c_str(), level_font_size, 2.0f);
        const Vector2 location_size = MeasureTextEx(_font, location_name.c_str(), location_font_size, 1.0f);

        DrawTextEx(_font, level_name.c_str(), {(screen_width - level_size.x) / 2.0f, banner_y + 20.0f}, level_font_size, 2.0f, withAlpha(COLOR_ACCENT, alpha));
        DrawTextEx(_font, location_name.c_str(), {(screen_width - location_size.x) / 2.0f, banner_y + 20.0f + level_size.y + 4.0f}, location_font_size, 1.0f, withAlpha(WHITE, alpha * 0.8f));

        const float line_width = Core::GlobalScaling::scaled(200.0f) * alpha;
        DrawLineEx({screen_width / 2.0f - line_width, banner_y}, {screen_width / 2.0f + line_width, banner_y}, 1.0f, withAlpha(COLOR_ACCENT, alpha * 0.5f));
        DrawLineEx({screen_width / 2.0f - line_width, banner_y + banner_height}, {screen_width / 2.0f + line_width, banner_y + banner_height}, 1.0f, withAlpha(COLOR_ACCENT, alpha * 0.5f));
    }
} // namespace Nawia::UI
