#include "UIHandler.h"
#include "menu/SettingsMenu.h"
#include "menu/LevelSelectMenu.h"
#include "StatsUI.h"

#include <Player.h>
#include <EntityManager.h>
#include <Entity.h>
#include <InteractiveClickable.h>
#include <Constants.h>
#include <GlobalScaling.h>
#include <Settings.h>
#include <Camera.h>
#include <Collider.h>
#include <ResourceManager.h>
#include <QuestManager.h>
#include <LevelManager.h>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace Nawia::UI {

    namespace {
        // Layout constants
        constexpr float MENU_SIDE_X_PCT = 0.10f;
        constexpr float MENU_START_Y_PCT = 0.42f;
        constexpr float BUTTON_WIDTH = 340.0f;
        constexpr float BUTTON_HEIGHT = 70.0f;
        constexpr float BUTTON_SPACING = 18.0f;

        std::vector<Rectangle> getVerticalMenuLayout(int count, bool centered = false) 
    	{
            const float screen_w = static_cast<float>(GetScreenWidth());
            const float screen_h = static_cast<float>(GetScreenHeight());
            const float b_w = Core::GlobalScaling::scaled(BUTTON_WIDTH);
            const float b_h = Core::GlobalScaling::scaled(BUTTON_HEIGHT);
            const float spacing = Core::GlobalScaling::scaled(BUTTON_SPACING);
            
            std::vector<Rectangle> rects;
            float start_x = centered ? (screen_w - b_w) / 2.0f : screen_w * MENU_SIDE_X_PCT;
            float start_y = centered ? (screen_h - (count * b_h + (count - 1) * spacing)) / 2.0f + Core::GlobalScaling::scaled(40.0f) : screen_h * MENU_START_Y_PCT;

            for (int i = 0; i < count; ++i) {
                rects.push_back({ start_x, start_y + i * (b_h + spacing), b_w, b_h });
            }
            return rects;
        }

        float fract(float v) { return v - std::floor(v); }
        float hash01(float s) { return fract(std::sin(s * 127.1f) * 43758.5453f); }
        Color withAlpha(Color c, float a) { return { c.r, c.g, c.b, static_cast<unsigned char>(std::clamp(a, 0.0f, 1.0f) * 255.0f) }; }
        Color LerpColor(Color c1, Color c2, float t) {
            return {
                static_cast<unsigned char>(c1.r + (c2.r - c1.r) * t),
                static_cast<unsigned char>(c1.g + (c2.g - c1.g) * t),
                static_cast<unsigned char>(c1.b + (c2.b - c1.b) * t),
                static_cast<unsigned char>(c1.a + (c2.a - c1.a) * t)
            };
        }

        void drawSmokeLayer(float w, float h, float t) {
            for (int i = 0; i < 26; ++i) {
                float s = static_cast<float>(i) * 11.73f + 3.1f;
                float travel = fract(hash01(s) + t * (0.012f + hash01(s + 2.0f) * 0.016f));
                float x = w * (0.05f + hash01(s + 1.0f) * 0.90f) + std::sin(t * (0.22f + hash01(s + 4.0f) * 0.18f) + s) * w * 0.06f;
                float y = h * (1.12f - travel * 1.24f);
                float radius = Core::GlobalScaling::scaled(110.0f + hash01(s + 5.0f) * 150.0f);
                float alpha = (0.35f + (1.0f - travel) * 0.65f) * (0.035f + hash01(s + 6.0f) * 0.07f);
                DrawCircleGradient(static_cast<int>(x), static_cast<int>(y), radius, withAlpha(LIGHTGRAY, alpha), withAlpha(DARKGRAY, 0.0f));
            }
        }

        void drawFireParticles(float w, float h, float t) {
            for (int i = 0; i < 96; ++i) {
                float s = static_cast<float>(i) * 17.13f + 8.0f;
                float cycle = fract(hash01(s) + t * (0.10f + hash01(s + 1.0f) * 0.22f));
                float rise = 1.0f - cycle;
                float x = w * (0.03f + hash01(s + 2.0f) * 0.94f) + std::sin(t * (1.0f + hash01(s + 3.0f) * 1.5f) + s) * w * (0.01f + hash01(s + 9.0f) * 0.02f);
                float y = h * (1.04f - rise * 1.18f);
                float radius = Core::GlobalScaling::scaled(1.5f + hash01(s + 7.0f) * hash01(s + 7.0f) * 12.0f) * (0.45f + rise * 0.95f);
                float alpha = (0.10f + rise * 0.50f) * (0.55f + hash01(s + 6.0f) * 0.45f);
                DrawCircleGradient(static_cast<int>(x), static_cast<int>(y), radius, withAlpha(Color{ 255, 233, 168, 255 }, alpha), withAlpha(Color{ 255, 96, 24, 255 }, alpha * 0.35f));
            }
        }
    }

    UIHandler::UIHandler() : _player(nullptr), _entity_manager(nullptr) {}
    UIHandler::~UIHandler() { UnloadFont(_font); }

    void UIHandler::initialize(const std::shared_ptr<Entity::Player>& player, Core::EntityManager* entity_manager, Core::ResourceManager& resource_manager, Game::QuestManager* quest_manager) {
        _player = player;
        _entity_manager = entity_manager;
        _font = LoadFontEx("../assets/fonts/slavic_font.ttf", Core::GlobalScaling::scaledInt(300), nullptr, 0);
        GenTextureMipmaps(&_font.texture);
        SetTextureFilter(_font.texture, TEXTURE_FILTER_TRILINEAR);

        _main_menu_bg = resource_manager.getTexture("../assets/textures/main_menu.png");
        _menu_btn_idle = resource_manager.getTexture("../assets/textures/ui/menu_btn_idle.png");
        _menu_btn_hover = resource_manager.getTexture("../assets/textures/ui/menu_btn_hover.png");

        _inventory_ui = std::make_unique<InventoryUI>();
        _inventory_ui->loadResources(resource_manager);
        _chest_ui = std::make_unique<ChestUI>();
        _stats_ui = std::make_unique<StatsUI>(_player);
        _quest_ui = std::make_unique<QuestUI>();
        _quest_manager = quest_manager;
        _previous_hp = _player->getHP();
    }

    void UIHandler::updateHoverTimers(float dt, const std::vector<Rectangle>& rects) {
        if (_hover_timers.size() != rects.size()) _hover_timers.assign(rects.size(), 0.0f);
        Vector2 mouse = GetMousePosition();
        for (size_t i = 0; i < rects.size(); ++i) {
            if (CheckCollisionPointRec(mouse, rects[i])) _hover_timers[i] = std::min(1.0f, _hover_timers[i] + dt * 6.0f);
            else _hover_timers[i] = std::max(0.0f, _hover_timers[i] - dt * 4.0f);
        }
    }

    void UIHandler::update(float dt) {
        if (_player) {
            if (_player->getHP() < _previous_hp) _damage_flash_timer = 0.3f;
            _previous_hp = _player->getHP();
        }
        if (_damage_flash_timer > 0.0f) _damage_flash_timer = std::max(0.0f, _damage_flash_timer - dt);

        // Determine which menu is active to update its hover timers
        if (_is_authors_open) updateHoverTimers(dt, getVerticalMenuLayout(1, true));
        else if (_settings_menu) {} // SettingsMenu handles its own hover for now
        else if (_level_select_menu) {}
        else updateHoverTimers(dt, getVerticalMenuLayout(4));

        for (auto it = _notifications.begin(); it != _notifications.end();) {
            it->timer -= dt;
            if (it->timer <= 0.0f) it = _notifications.erase(it); else ++it;
        }
    }

    void UIHandler::renderVerticalMenu(const char* title, const std::vector<MenuButtonDef>& buttons, bool centered) const {
        const float screen_w = static_cast<float>(GetScreenWidth());
        const float screen_h = static_cast<float>(GetScreenHeight());
        const float spacing = Core::GlobalScaling::scaled(2.0f);
        
        // Title
        const float title_fs = Core::GlobalScaling::scaled(centered ? FONT_SIZE_TITLE : 140.0f);
        Vector2 title_size = MeasureTextEx(_font, title, title_fs, spacing);
        Vector2 title_pos = centered ? Vector2{(screen_w - title_size.x) / 2.0f, Core::GlobalScaling::scaled(80.0f)} 
                                     : Vector2{screen_w * MENU_SIDE_X_PCT, screen_h * 0.22f + std::sin(static_cast<float>(GetTime()) * 0.8f) * Core::GlobalScaling::scaled(4.0f)};
        
        if (!centered) { // Special main menu title style
            DrawTextEx(_font, title, { title_pos.x + 6, title_pos.y + 6 }, title_fs, spacing, withAlpha(BLACK, 0.8f));
            DrawTextEx(_font, title, title_pos, title_fs, spacing, WHITE);
        } else {
            DrawTextEx(_font, title, title_pos, title_fs, spacing, Color{ 255, 200, 100, 255 });
        }

        auto rects = getVerticalMenuLayout(static_cast<int>(buttons.size()), centered);
        for (size_t i = 0; i < buttons.size(); ++i) {
            float hover = (i < _hover_timers.size()) ? _hover_timers[i] : 0.0f;
            drawMenuButton(rects[i], buttons[i].label, hover);
        }
    }

    void UIHandler::renderMainMenu() const {
        if (_is_authors_open) { renderAuthorsMenu(); return; }
        drawSharedMenuBackground();
        renderVerticalMenu("Nawia", { {"GRAJ", MenuAction::Play}, {"USTAWIENIA", MenuAction::Settings}, {"AUTORZY", MenuAction::Authors}, {"WYJDZ", MenuAction::Exit} });
    }

    void UIHandler::renderAuthorsMenu() const {
        drawSharedMenuBackground();
        const float screen_w = static_cast<float>(GetScreenWidth());
        const float screen_h = static_cast<float>(GetScreenHeight());
        const float fs = Core::GlobalScaling::scaled(FONT_SIZE_SUBTITLE);
        const char* names[] = { "Krzysztof", "Marcin", "Szymon", "Wojciech" };
        float cy = screen_h * 0.35f;
        for (const auto* n : names) {
            Vector2 sz = MeasureTextEx(_font, n, fs, 2.0f);
            DrawTextEx(_font, n, {(screen_w - sz.x) / 2.0f, cy}, fs, 2.0f, WHITE);
            cy += fs + Core::GlobalScaling::scaled(30.0f);
        }
        renderVerticalMenu("AUTORZY", { {"POWROT", MenuAction::None} }, true);
    }

    MenuAction UIHandler::handleMenuInput() {
        bool is_centered = _is_authors_open;
        int count = is_centered ? 1 : 4;
        auto rects = getVerticalMenuLayout(count, is_centered);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            for (size_t i = 0; i < rects.size(); ++i) {
                if (CheckCollisionPointRec(m, rects[i])) {
                    if (_is_authors_open) { _is_authors_open = false; return MenuAction::None; }
                    return (i == 0) ? MenuAction::Play : (i == 1) ? MenuAction::Settings : (i == 2) ? MenuAction::Authors : MenuAction::Exit;
                }
            }
        }
        if (IsKeyPressed(KEY_ESCAPE) && _is_authors_open) _is_authors_open = false;
        return MenuAction::None;
    }

    void UIHandler::drawMenuButton(const Rectangle& rect, const char* text, float hover) const {
        DrawRectangleRec(rect, withAlpha({ 40, 40, 50, 255 }, 0.45f + hover * 0.35f));
        DrawRectangleLinesEx(rect, Core::GlobalScaling::scaled(2.0f), LerpColor(withAlpha(WHITE, 0.35f), withAlpha(Color{ 255, 200, 100, 255 }, 0.9f), hover));
        if (hover > 0.01f) DrawRectangleGradientV(rect.x, rect.y, rect.width, rect.height, withAlpha(WHITE, 0.06f * hover), withAlpha(WHITE, 0.0f));
        float fs = Core::GlobalScaling::scaled(FONT_SIZE_BUTTON);
        Vector2 sz = MeasureTextEx(_font, text, fs, 2.0f);
        Vector2 pos = { rect.x + (rect.width - sz.x) / 2.0f + hover * Core::GlobalScaling::scaled(12.0f), rect.y + (rect.height - sz.y) / 2.0f };
        DrawTextEx(_font, text, { pos.x + 2, pos.y + 2 }, fs, 2.0f, withAlpha(BLACK, 0.5f));
        DrawTextEx(_font, text, pos, fs, 2.0f, LerpColor(withAlpha(WHITE, 0.95f), { 255, 225, 120, 255 }, hover));
    }

    void UIHandler::drawSharedMenuBackground() const {
        float w = GetScreenWidth(), h = GetScreenHeight(), t = GetTime();
        if (_main_menu_bg) {
            float ta = static_cast<float>(_main_menu_bg->width) / _main_menu_bg->height, sa = w / h, sw = _main_menu_bg->width, sh = _main_menu_bg->height;
            if (ta > sa) sw = sh * sa; else sh = sw / sa;
            float z = 0.11f + 0.02f * std::sin(t * 0.23f); sw *= (1.0f - z); sh *= (1.0f - z);
            float ox = std::max(0.0f, (_main_menu_bg->width - sw) * 0.5f) * std::sin(t * 0.12f + 0.8f);
            float oy = std::max(0.0f, (_main_menu_bg->height - sh) * 0.5f) * std::cos(t * 0.09f - 0.35f);
            DrawTexturePro(*_main_menu_bg, { (_main_menu_bg->width - sw) * 0.5f + ox, (_main_menu_bg->height - sh) * 0.5f + oy, sw, sh }, { 0, 0, w, h }, { 0, 0 }, 0.0f, WHITE);
        } else DrawRectangle(0, 0, w, h, Fade(BLACK, 0.8f));
        DrawRectangleGradientV(0, 0, w, h, withAlpha({ 30, 14, 10, 255 }, 0.10f), withAlpha({ 5, 5, 8, 255 }, 0.48f));
        drawSmokeLayer(w, h, t); drawFireParticles(w, h, t);
        DrawRectangleGradientV(0, 0, w, h, withAlpha(Color{ 255, 170, 100, 255 }, 0.02f), withAlpha(Color{ 0, 0, 0, 255 }, 0.12f));
    }

    void UIHandler::render(const Core::GameCamera& camera) {
        if (!_player || !_entity_manager) return;
        renderPlayerHealthBar(); renderPlayerAbilityBar(); renderPlayerExperienceBar();
        renderCombatEntityHealthBars(camera); renderLocationInfo();
        if (_damage_flash_timer > 0.0f) DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RED, (_damage_flash_timer / 0.3f) * 0.4f));
        float ny = 10.0f;
        for (const auto& n : _notifications) {
            Vector2 sz = MeasureTextEx(_font, n.text.c_str(), Core::GlobalScaling::scaled(24.0f), 1.0f);
            float x = GetScreenWidth() - sz.x - 20.0f;
            DrawRectangle(x - 5, ny - 5, sz.x + 10, sz.y + 10, Fade(BLACK, 0.7f));
            DrawRectangleLines(x - 5, ny - 5, sz.x + 10, sz.y + 10, WHITE);
            DrawTextEx(_font, n.text.c_str(), { x, ny }, Core::GlobalScaling::scaled(24.0f), 1.0f, WHITE);
            ny += sz.y + 20.0f;
        }
        _dialogueUI.render(_font);
        if (_is_inventory_open) {
            _inventory_ui->render(_font, *_player);
            if (_stats_ui) _stats_ui->render(20.0f, GetScreenHeight() - Core::GlobalScaling::scaled(260.0f), _font);
            if (_current_container) _chest_ui->render(*_current_container->getInventory(), _font);
        }
        if (_is_quest_ui_open) _quest_ui->render(_font, _quest_manager);
    }

    void UIHandler::renderPauseMenu() const {
        drawSharedMenuBackground(); // Shared background even for pause if we want it unified
        renderVerticalMenu("PAUZA", { {"KONTYNUUJ", MenuAction::Play}, {"USTAWIENIA", MenuAction::Settings}, {"MENU GLOWNE", MenuAction::Exit} }, true);
    }

    MenuAction UIHandler::handlePauseMenuInput() {
        auto rects = getVerticalMenuLayout(3, true);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            if (CheckCollisionPointRec(m, rects[0])) return MenuAction::Play;
            if (CheckCollisionPointRec(m, rects[1])) return MenuAction::Settings;
            if (CheckCollisionPointRec(m, rects[2])) return MenuAction::Exit;
        }
        return MenuAction::None;
    }

    void UIHandler::renderGameOverScreen() const {
        drawSharedMenuBackground();
        renderVerticalMenu("NIE ZYJESZ", { {"ODRODZENIE", MenuAction::Respawn}, {"MENU GLOWNE", MenuAction::Exit} }, true);
    }

    MenuAction UIHandler::handleGameOverInput() {
        auto rects = getVerticalMenuLayout(2, true);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            if (CheckCollisionPointRec(m, rects[0])) return MenuAction::Respawn;
            if (CheckCollisionPointRec(m, rects[1])) return MenuAction::Exit;
        }
        return MenuAction::None;
    }

    // Remaining basic helpers
    void UIHandler::drawBar(float x, float y, float w, float h, float p, Color fg, Color bg) const { DrawRectangle(x, y, w, h, bg); DrawRectangle(x, y, w * p, h, fg); }
    void UIHandler::showNotification(const std::string& t, float d) { _notifications.push_back({ t, d, d }); }
    void UIHandler::handleInput() {
        if (_dialogueUI.isOpen()) { _dialogueUI.handleInput(); return; }
        if (IsKeyPressed(KEY_I) || IsKeyPressed(KEY_TAB)) { if (_current_container) closeContainer(); toggleInventory(); }
        if (IsKeyPressed(KEY_P)) toggleQuestUI();
        if (_is_quest_ui_open) _quest_ui->handleInput();
        if (_is_inventory_open) {
            int bs = _inventory_ui->handleInput(); if (bs != -1) { _player->equipItemFromBackpack(bs); return; }
            auto es = _inventory_ui->getClickedEquipmentSlot(); if (es != Item::EquipmentSlot::None) _player->unequipItem(es);
            if (_current_container) {
                int cs = _chest_ui->handleInput(); if (cs != -1) { auto& ci = *_current_container->getInventory(); auto item = ci.getItem(cs);
                    if (item && _player->getBackpack().addItem(item)) { ci.removeItem(cs); if (_quest_manager) _quest_manager->notifyItemCollected(item->getId()); }
                }
            }
        }
    }
    void UIHandler::renderPlayerHealthBar() const {
        float w = Core::GlobalScaling::scaled(300.0f), h = Core::GlobalScaling::scaled(25.0f), x = (GetScreenWidth() - w) / 2.0f, y = GetScreenHeight() - Core::GlobalScaling::scaled(75.0f);
        drawBar(x, y, w, h, std::clamp(static_cast<float>(_player->getHP()) / _player->getMaxHP(), 0.0f, 1.0f), RED, DARKGRAY);
        DrawRectangleLinesEx({ x, y, w, h }, 2.0f, WHITE);
        const char* t = TextFormat("%d / %d", _player->getHP(), _player->getMaxHP()); Vector2 sz = MeasureTextEx(_font, t, 20.0f, 1.0f);
        DrawTextEx(_font, t, { x + (w - sz.x) / 2.0f, y + (h - sz.y) / 2.0f }, 20.0f, 1.0f, WHITE);
    }
    void UIHandler::renderCombatEntityHealthBars(const Core::GameCamera& camera) const {
        for (const auto& e : _entity_manager->getEntities()) if (!e->isDormant() && (e->getFaction() == Entity::Faction::Enemy || e->getFaction() == Entity::Faction::Ally) && e->getHP() < e->getMaxHP() && e->getHP() > 0) {
            Vector2 sp = e->getScreenPosition(camera.get()); float w = 40.0f, h = 6.0f, x = sp.x - w / 2.0f, y = sp.y - 60.0f * Core::GlobalScaling::getScale();
            drawBar(x, y, w, h, std::clamp(static_cast<float>(e->getHP()) / e->getMaxHP(), 0.0f, 1.0f), RED, DARKGRAY); DrawRectangleLinesEx({ x, y, w, h }, 1.0f, BLACK);
        }
    }
    void UIHandler::renderPlayerAbilityBar() const {
        const auto& ab = _player->getAbilities(); float is = 50.0f, sp = 10.0f, w = (is * 4) + (sp * 3), sx = (GetScreenWidth() - w) / 2.0f, sy = GetScreenHeight() - 140.0f;
        for (int i = 0; i < 4; ++i) { float x = sx + (is + sp) * i; Rectangle r = { x, sy, is, is }; DrawRectangleRec(r, Fade(BLACK, 0.5f)); DrawRectangleLinesEx(r, 2.0f, DARKGRAY);
            if (i >= ab.size()) continue; auto& a = ab[i]; if (auto ic = a->getIcon()) DrawTexturePro(*ic, { 0, 0, (float)ic->width, (float)ic->height }, r, { 0, 0 }, 0.0f, WHITE);
            if (!a->isReady()) { float cr = a->getCooldownTimer() / a->getStats().cooldown; DrawRectangle(x, sy, is, is * cr, Fade(GRAY, 0.8f));
                const char* t = TextFormat("%.1f", a->getCooldownTimer()); Vector2 sz = MeasureTextEx(_font, t, 20.0f, 1.0f); DrawTextEx(_font, t, { x + (is - sz.x) / 2.0f, sy + (is - sz.y) / 2.0f }, 20.0f, 1.0f, WHITE);
            }
        }
    }
    void UIHandler::renderPlayerExperienceBar() const {
        float w = (50.0f * 4) + (10.0f * 3), h = 10.0f, cr = 18.0f, sy = GetScreenHeight() - 152.0f - h, bx = (GetScreenWidth() - w) / 2.0f, cx = bx - 8.0f - cr, cy = sy + h / 2.0f;
        int exp = _player->getExp(), next = std::max(1, _player->getExpToNextLvl()); drawBar(bx, sy, w, h, std::clamp((float)exp / next, 0.0f, 1.0f), PURPLE, DARKGRAY);
        DrawRectangleLinesEx({ bx, sy, w, h }, 2.0f, WHITE); DrawCircle(cx, cy, cr, DARKGRAY); DrawCircleLines(cx, cy, cr, WHITE);
        const char* t = TextFormat("%d", _player->getLevel()); Vector2 sz = MeasureTextEx(_font, t, 20.0f, 1.0f); DrawTextEx(_font, t, { cx - sz.x / 2.0f, cy - sz.y / 2.0f }, 20.0f, 1.0f, WHITE);
    }
    void UIHandler::renderLocationInfo() const {
        if (!_level_manager) return; std::string ln = _level_manager->getCurrentLevelName(), loc = _level_manager->getCurrentLocationName(); if (ln.empty()) return;
        float fs1 = 20.0f, fs2 = 16.0f, p = 10.0f, m = 40.0f; Vector2 s1 = MeasureTextEx(_font, ln.c_str(), fs1, 1.0f), s2 = MeasureTextEx(_font, loc.c_str(), fs2, 1.0f);
        float bw = std::max(s1.x, s2.x) + p * 2.0f, bh = s1.y + s2.y + p * 2.5f; DrawRectangle(m, m, bw, bh, Fade(BLACK, 0.6f)); DrawRectangleLinesEx({m, m, bw, bh}, 1.0f, Fade(WHITE, 0.3f));
        DrawTextEx(_font, ln.c_str(), {m + p, m + p}, fs1, 1.0f, WHITE); DrawTextEx(_font, loc.c_str(), {m + p + 5, m + p + s1.y + 4}, fs2, 1.0f, Fade(WHITE, 0.7f));
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
    std::string UIHandler::handleLevelSelectInput() { if (_level_select_menu) return _level_select_menu->handleInput(); return ""; }
    void UIHandler::openContainer(Entity::InteractiveClickable* c) { _current_container = c; _is_inventory_open = true; }
    void UIHandler::closeContainer() { _current_container = nullptr; }
    bool UIHandler::isInputBlocked() const { return _dialogueUI.isOpen() || _is_inventory_open || _current_container || _is_quest_ui_open; }
} // namespace Nawia::UI
