#pragma once

#include <SaveGameManager.h>

#include <raylib.h>
#include <vector>

namespace Nawia::UI {

    class UIHandler;

    /**
     * @class SaveSlotMenu
     * @brief Panel wyboru slotu zapisu w trzech wariantach: zapis, wczytanie, slot startowy.
     *
     * Layout korzysta z kart o tym samym rozmiarze co lista poziomow, dzieki
     * czemu mieszcza sie data zapisu, lokacja i nazwa poziomu bez ucinania.
     */
    class SaveSlotMenu {
    public:
        /**
         * @enum Mode
         * @brief Cel otwarcia menu slotow.
         */
        enum class Mode {
            Load,           ///< Wczytanie istniejacego zapisu.
            Save,           ///< Zapis biezacej gry, z potwierdzeniem nadpisania.
            SelectDefault   ///< Wybor slotu startowego dla nowej gry.
        };

        SaveSlotMenu(std::vector<Game::SaveSlotInfo> slots, Mode mode);

        /** @brief Rysuje karty slotow oraz dialog nadpisania, gdy aktywny. */
        void render(const UIHandler& ui) const;

        /**
         * @brief Obsluguje wejscie gracza i zwraca wynik interakcji.
         * @return Numer wybranego slotu (>0), -1 dla anulowania albo 0 dla braku akcji.
         */
        [[nodiscard]] int handleInput();

        /** @brief Zwraca, w jakim trybie zostalo otwarte menu. */
        [[nodiscard]] Mode getMode() const { return _mode; }

    private:
        [[nodiscard]] static std::vector<Rectangle> buildCardLayout(int slot_count);
        [[nodiscard]] static Rectangle getBackButtonRect();
        [[nodiscard]] static Rectangle getModalButtonRect(int index);

        void drawSlotCard(const Rectangle& rect, const Game::SaveSlotInfo& slot, bool hovered, const Font& font) const;

        std::vector<Game::SaveSlotInfo> _slots;
        Mode _mode = Mode::Load;
        int _pending_overwrite_slot = 0;
    };

} // namespace Nawia::UI
