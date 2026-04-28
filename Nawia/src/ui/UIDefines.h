#pragma once
#include <raylib.h>

namespace Nawia::UI {

    // --- Font Sizes ---
    constexpr float FONT_SIZE_MAIN_TITLE = 140.0f;
    constexpr float FONT_SIZE_TITLE = 80.0f;
    constexpr float FONT_SIZE_SUBTITLE = 40.0f;
    constexpr float FONT_SIZE_BUTTON = 24.0f;
    constexpr float FONT_SIZE_TEXT = 18.0f;
    constexpr float FONT_SIZE_HUD_SMALL = 20.0f;

    // --- Menu Layout ---
    constexpr float MENU_SIDE_X_PCT = 0.10f;
    constexpr float MENU_START_Y_PCT = 0.42f;
    constexpr float BUTTON_WIDTH = 340.0f;
    constexpr float BUTTON_HEIGHT = 70.0f;
    constexpr float BUTTON_SPACING = 18.0f;
    constexpr float BACK_BUTTON_BOTTOM_OFFSET = 140.0f;
    constexpr float SIDEBAR_WIDTH = 280.0f;
    constexpr float PANEL_MARGIN = 40.0f;

    // --- Particles & FX ---
    constexpr int SMOKE_LAYER_COUNT = 26;
    constexpr int FIRE_PARTICLE_COUNT = 96;

    // --- Menu Labels ---
    inline const char* LABEL_PLAY = "GRAJ";
    inline const char* LABEL_SETTINGS = "USTAWIENIA";
    inline const char* LABEL_AUTHORS = "AUTORZY";
    inline const char* LABEL_EXIT = "WYJDZ";
    inline const char* LABEL_BACK = "POWROT";
    inline const char* LABEL_APPLY = "ZATWIERDZ";
    inline const char* LABEL_RESPAWN = "ODRODZENIE";
    inline const char* LABEL_MAIN_MENU = "MENU GLOWNE";
    inline const char* LABEL_PAUSE = "PAUZA";
    inline const char* LABEL_GAME_OVER = "NIE ZYJESZ";
    inline const char* LABEL_SELECT_LEVEL = "WYBIERZ POZIOM";
    inline const char* LABEL_CONTINUE = "KONTYNUUJ";

    // --- Colors (Premium Palette) ---
    inline const Color COLOR_ACCENT = { 255, 200, 100, 255 };          // Gold
    inline const Color COLOR_ACCENT_SOFT = { 255, 200, 100, 50 };      // Soft Gold
    inline const Color COLOR_GOLDEN_TEXT = { 255, 225, 120, 255 };     // Bright Gold
    inline const Color COLOR_PANEL_BG = { 40, 40, 50, 255 };           // Dark Slate
    inline const Color COLOR_SLAVIC_ORANGE = { 255, 120, 40, 255 };    // Embers
    inline const Color COLOR_BLACK_GLASS = { 0, 0, 0, 128 };           // Glassmorphism
    inline const Color COLOR_WHITE_GLASS = { 255, 255, 255, 25 };      // Hover Glass

    // --- HUD & Bars ---
    constexpr float BAR_HEALTH_WIDTH = 300.0f;
    constexpr float BAR_HEALTH_HEIGHT = 25.0f;
    constexpr float BAR_EXP_HEIGHT = 10.0f;
    constexpr float HUD_MARGIN_BOTTOM = 75.0f;
    constexpr float ABILITY_ICON_SIZE = 50.0f;
    constexpr float ABILITY_SPACING = 10.0f;
    constexpr float EXP_CIRCLE_RADIUS = 18.0f;

    // --- Authors ---
    inline const char* AUTHOR_NAME_1 = "Michal Sadkowski";
    inline const char* AUTHOR_NAME_2 = "Michal Matysiak";
    inline const char* AUTHOR_NAME_3 = "Dawid Wesolowski";
    inline const char* AUTHOR_NAME_4 = "Ostap Lozovyy";

} // namespace Nawia::UI
