#pragma once

#include <raylib.h>

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace Nawia::Audio {

	/**
	 * @enum PlaylistMode
	 * @brief Okresla kolejnosc odtwarzania playlisty muzycznej.
	 */
	enum class PlaylistMode {
		Sequential,
		Random
	};

	/**
	 * @struct SoundOptions
	 * @brief Opcje jednorazowego odtworzenia efektu dzwiekowego.
	 */
	struct SoundOptions {
		float volume = 1.0f;
		float pitch = 1.0f;
		bool restart_if_playing = true;
	};

	/**
	 * @struct PlaylistOptions
	 * @brief Opcje odtwarzania listy utworow muzycznych.
	 */
	struct PlaylistOptions {
		PlaylistMode mode = PlaylistMode::Sequential;
		bool loop = true;
		float volume = 1.0f;
	};

	/**
	 * @class AudioManager
	 * @brief Zarzadza efektami dzwiekowymi, muzyka, playlistami i glosnoscia.
	 *
	 * Efekty dzwiekowe sa cache'owane jako `Sound`, a muzyka jest streamowana jako
	 * `Music`, dlatego `update()` musi byc wolane raz na klatke.
	 */
	class AudioManager {
	public:
		AudioManager();
		~AudioManager();

		AudioManager(const AudioManager&) = delete;
		AudioManager& operator=(const AudioManager&) = delete;

		/** @brief Inicjalizuje urzadzenie audio Raylib. */
		bool initialize();

		/** @brief Zwalnia dzwieki, muzyke i urzadzenie audio. */
		void shutdown();

		/** @brief Aktualizuje streaming muzyki i przejscia playlisty. */
		void update();

		/**
		 * @brief Laduje efekt dzwiekowy do cache pod wskazanym identyfikatorem.
		 * @param id Stabilny identyfikator uzywany przez gameplay.
		 * @param path Sciezka runtime do pliku audio.
		 */
		bool loadSound(const std::string& id, const std::string& path);

		/**
		 * @brief Odtwarza efekt dzwiekowy z cache.
		 * @param id Identyfikator efektu.
		 * @param options Glosnosc, pitch i zachowanie przy juz grajacym dzwieku.
		 */
		bool playSound(const std::string& id, SoundOptions options = {});

		/** @brief Laduje i odtwarza efekt dzwiekowy jednym wywolaniem. */
		bool playSoundFile(const std::string& id, const std::string& path, SoundOptions options = {});

		/** @brief Zatrzymuje efekt dzwiekowy, jesli jest aktualnie odtwarzany. */
		void stopSound(const std::string& id);

		/**
		 * @brief Odtwarza pojedynczy utwor muzyczny.
		 * @param path Sciezka runtime do pliku muzyki.
		 * @param loop Czy utwor ma zapetlac sie bez playlisty.
		 * @param volume Glosnosc lokalna utworu w zakresie 0..1.
		 */
		bool playMusic(const std::string& path, bool loop = true, float volume = 1.0f);

		/** @brief Odtwarza playliste muzyczna sekwencyjnie albo losowo. */
		bool playPlaylist(const std::vector<std::string>& tracks, PlaylistOptions options = {});

		/** @brief Zatrzymuje i zwalnia aktualna muzyke. */
		void stopMusic();

		/** @brief Pauzuje aktualna muzyke bez zwalniania streamu. */
		void pauseMusic();

		/** @brief Wznawia zapauzowana muzyke. */
		void resumeMusic();

		[[nodiscard]] bool isMusicPlaying() const;

		void setMasterVolume(float volume);
		void setMusicVolume(float volume);
		void setEffectsVolume(float volume);

		[[nodiscard]] float getMasterVolume() const { return _master_volume; }
		[[nodiscard]] float getMusicVolume() const { return _music_volume; }
		[[nodiscard]] float getEffectsVolume() const { return _effects_volume; }

	private:
		struct CachedSound {
			Sound sound;
			std::string path;
		};

		bool loadAndPlayMusic(const std::string& path, float volume);
		void unloadCurrentMusic();
		void advancePlaylist();
		void preparePlaylistOrder();
		void updateMusicVolume() const;
		static float clampVolume(float volume);

		bool _is_initialized = false;
		float _master_volume = 1.0f;
		float _music_volume = 0.7f;
		float _effects_volume = 1.0f;

		std::unordered_map<std::string, CachedSound> _sounds;

		Music _current_music = {};
		bool _has_music = false;
		bool _current_music_loop = false;
		float _current_music_volume = 1.0f;
		std::string _current_music_path;

		std::vector<std::string> _playlist;
		std::vector<size_t> _playlist_order;
		size_t _playlist_order_position = 0;
		PlaylistOptions _playlist_options;
		std::mt19937 _random_engine;
	};

} // namespace Nawia::Audio
