#pragma once

#include <Dialogue.h>

#include <raylib.h>

#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace Nawia::Audio { class AudioManager; }

namespace Nawia::UI {

    /**
     * @class DialogueUI
     * @brief Rysuje aktywny dialog i zarzadza przejsciami miedzy wezlami.
     */
    class DialogueUI {
    public:
        /** @brief Otwiera drzewo dialogowe od pierwszego wezla. */
        void open(const Game::DialogueTree& tree, int start_node_id = 0, std::function<void(int, bool)> on_close = nullptr);

        /** @brief Ustawia manager audio uzywany do odtwarzania glosow dialogowych. */
        void setAudioManager(Audio::AudioManager* audio_manager) { _audio_manager = audio_manager; }

        /** @brief Zamyka aktualny dialog. */
        void close(bool completed = false);

        /** @brief Rysuje aktualny wezel dialogowy z dynamicznie dobrana wysokoscia. */
        void render(const Font& font);

        /** @brief Obsluguje klikniecia w opcje dialogowe. */
        bool handleInput();

        /** @brief Zwraca, czy dialog jest aktualnie otwarty. */
        [[nodiscard]] bool isOpen() const { return _is_open; }
        [[nodiscard]] int getCurrentNodeId() const { return _current_node_id; }

    private:
        bool _is_open = false;
        Game::DialogueTree _current_tree;
        int _current_node_id = 0;
        std::function<void(int, bool)> _on_close = nullptr;
        std::vector<Rectangle> _option_rectangles;
        Audio::AudioManager* _audio_manager = nullptr;
        std::string _current_voice_id;

        void playCurrentNodeVoice();
        void stopCurrentVoice();
    };

} // namespace Nawia::UI
