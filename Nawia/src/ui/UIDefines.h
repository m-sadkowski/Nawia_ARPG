#pragma once

#include <raylib.h>

namespace Nawia::UI {

    // Rozmiary czcionek.
    constexpr float FONT_SIZE_MAIN_TITLE = 140.0f;
    constexpr float FONT_SIZE_TITLE = 80.0f;
    constexpr float FONT_SIZE_SUBTITLE = 40.0f;
    constexpr float FONT_SIZE_BUTTON = 24.0f;
    constexpr float FONT_SIZE_TEXT = 18.0f;
    constexpr float FONT_SIZE_HUD_SMALL = 20.0f;

    // Uklad menu.
    constexpr float MENU_SIDE_X_PCT = 0.10f;
    constexpr float MENU_START_Y_PCT = 0.42f;
    constexpr float BUTTON_WIDTH = 340.0f;
    constexpr float BUTTON_HEIGHT = 70.0f;
    constexpr float BUTTON_SPACING = 18.0f;
    constexpr float BACK_BUTTON_BOTTOM_OFFSET = 140.0f;
    constexpr float SIDEBAR_WIDTH = 280.0f;
    constexpr float PANEL_MARGIN = 40.0f;

    // Czasteczki i efekty.
    constexpr int SMOKE_LAYER_COUNT = 26;
    constexpr int FIRE_PARTICLE_COUNT = 96;

    // Etykiety menu.
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

    // Kolory interfejsu.
    inline const Color COLOR_ACCENT = { 255, 200, 100, 255 };          // Zloto.
    inline const Color COLOR_ACCENT_SOFT = { 255, 200, 100, 50 };      // Delikatne zloto.
    inline const Color COLOR_GOLDEN_TEXT = { 255, 225, 120, 255 };     // Jasny zloty tekst.
    inline const Color COLOR_PANEL_BG = { 40, 40, 50, 255 };           // Ciemne tlo paneli.
    inline const Color COLOR_SLAVIC_ORANGE = { 255, 120, 40, 255 };    // Zar ognia.
    inline const Color COLOR_SLAVIC_BLUE = { 100, 180, 255, 255 };     // Blekit duchowy.
    inline const Color COLOR_HEALTH_GHOST = { 255, 255, 255, 180 };    // Cien poprzedniego HP.
    inline const Color COLOR_BLACK_GLASS = { 0, 0, 0, 128 };           // Przyciemnione szklo.
    inline const Color COLOR_WHITE_GLASS = { 255, 255, 255, 25 };      // Jasne szklo pod hover.
    inline const Color COLOR_PARCHMENT = { 240, 230, 200, 255 };       // Tekst pergaminowy.

    // HUD i paski.
    constexpr float BAR_HEALTH_WIDTH = 300.0f;
    constexpr float BAR_HEALTH_HEIGHT = 25.0f;
    constexpr float BAR_EXP_HEIGHT = 10.0f;
    constexpr float HUD_MARGIN_BOTTOM = 75.0f;
    constexpr float ABILITY_ICON_SIZE = 50.0f;
    constexpr float ABILITY_SPACING = 10.0f;
    constexpr float EXP_CIRCLE_RADIUS = 18.0f;
    constexpr float DIALOGUE_BOX_HEIGHT = 140.0f; // Minimalna wysokosc dialogu.
    constexpr float DIALOGUE_BOX_MARGIN = 80.0f;  // Odstep od dolu ekranu.

    // Autorzy.
    inline const char* AUTHOR_NAME_1 = "Michal Sadkowski";
    inline const char* AUTHOR_NAME_2 = "Michal Matysiak";
    inline const char* AUTHOR_NAME_3 = "Dawid Wesolowski";
    inline const char* AUTHOR_NAME_4 = "Ostap Lozovyy";

} // namespace Nawia::UI
