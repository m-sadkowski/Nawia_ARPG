#pragma once

#include <string>
#include <vector>
#include <fstream>

namespace Nawia::Core {

/**
 * @struct Resolution
 * @brief Opisuje rozdzielczosc ekranu.
 */
struct Resolution {
    int width;
    int height;
    
    [[nodiscard]] std::string toString() const {
        return std::to_string(width) + " x " + std::to_string(height);
    }
    
    bool operator==(const Resolution& other) const {
        return width == other.width && height == other.height;
    }
};

/**
 * @enum TextureQuality
 * @brief Dostepne poziomy jakosci tekstur.
 */
enum class TextureQuality { Low, Medium, High };

/**
 * @class Settings
 * @brief Przechowuje i zapisuje ustawienia gry.
 */
class Settings {
public:
    /// Aktualna rozdzielczosc.
    Resolution resolution = {1280, 720};
    
    /// Tryb pelnoekranowy.
    bool fullscreen = false;
    
    /// Reczna skala interfejsu.
    float ui_scale = 1.0f;
    
    /// Jakosc tekstur.
    TextureQuality texture_quality = TextureQuality::Medium;

    /// Czy pokazywac licznik FPS.
    bool show_fps = false;
    
    /// Limity skali interfejsu.
    static constexpr float UI_SCALE_MIN = 0.5f;
    static constexpr float UI_SCALE_MAX = 1.5f;
    static constexpr float UI_SCALE_STEP = 0.1f;
    
    /// Dostepne presety rozdzielczosci.
    static inline const std::vector<Resolution> AVAILABLE_RESOLUTIONS = {
        {1280, 720},
        {1366, 768},
        {1600, 900},
        {1920, 1080},
        {2560, 1440}
    };
    
    /// Domyslna sciezka pliku ustawien.
    static constexpr const char* DEFAULT_PATH = "assets/settings.json";
    
    /**
     * @brief Wczytuje ustawienia z pliku JSON.
     */
    bool load(const std::string& filepath = DEFAULT_PATH);
    
    /**
     * @brief Zapisuje ustawienia do pliku JSON.
     */
    [[nodiscard]] bool save(const std::string& filepath = DEFAULT_PATH) const;
    
    /**
     * @brief Zwraca indeks aktualnej rozdzielczosci.
     */
    [[nodiscard]] int getCurrentResolutionIndex() const;
    
    /**
     * @brief Ustawia rozdzielczosc na podstawie indeksu.
     */
    void setResolutionByIndex(int index);

    /**
     * @brief Zwraca nazwe jakosci tekstur po polsku.
     */
    [[nodiscard]] std::string getTextureQualityString() const {
        switch (texture_quality) {
            case TextureQuality::Low: return "Niska";
            case TextureQuality::Medium: return "Srednia";
            case TextureQuality::High: return "Wysoka";
            default: return "Nieznana";
        }
    }
};

} // namespace Nawia::Core
