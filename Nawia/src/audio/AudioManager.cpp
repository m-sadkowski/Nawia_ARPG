#include "AudioManager.h"

#include <Logger.h>

#include <algorithm>
#include <numeric>

namespace Nawia::Audio {

	namespace {

		constexpr float MUSIC_END_EPSILON = 0.05f;

	}

	AudioManager::AudioManager() : _random_engine(std::random_device{}()) {}

	AudioManager::~AudioManager() {
		shutdown();
	}

	bool AudioManager::initialize() {
		if (_is_initialized)
			return true;

		InitAudioDevice();
		if (!IsAudioDeviceReady()) {
			Core::Logger::errorLog("AudioManager: nie udalo sie zainicjalizowac urzadzenia audio.");
			return false;
		}

		SetMasterVolume(_master_volume);
		_is_initialized = true;
		Core::Logger::debugLog("AudioManager: zainicjalizowano urzadzenie audio.");
		return true;
	}

	void AudioManager::shutdown() {
		if (!_is_initialized)
			return;

		unloadCurrentMusic();

		for (auto& [id, cached_sound] : _sounds)
			UnloadSound(cached_sound.sound);
		_sounds.clear();

		CloseAudioDevice();
		_is_initialized = false;
	}

	void AudioManager::update() {
		if (!_is_initialized || !_has_music)
			return;

		UpdateMusicStream(_current_music);

		const float length = GetMusicTimeLength(_current_music);
		const float played = GetMusicTimePlayed(_current_music);
		if (length <= 0.0f || played < length - MUSIC_END_EPSILON)
			return;

		if (!_playlist.empty()) {
			advancePlaylist();
			return;
		}

		if (_current_music_loop) {
			SeekMusicStream(_current_music, 0.0f);
			PlayMusicStream(_current_music);
			return;
		}

		stopMusic();
	}

	bool AudioManager::loadSound(const std::string& id, const std::string& path) {
		if (!_is_initialized || id.empty() || path.empty())
			return false;

		if (_sounds.contains(id))
			return true;

		const Sound sound = LoadSound(path.c_str());
		if (sound.frameCount == 0) {
			Core::Logger::errorLog("AudioManager: nie udalo sie zaladowac efektu: " + path);
			return false;
		}

		_sounds.emplace(id, CachedSound{sound, path});
		return true;
	}

	bool AudioManager::playSound(const std::string& id, SoundOptions options) {
		if (!_is_initialized)
			return false;

		const auto sound_iterator = _sounds.find(id);
		if (sound_iterator == _sounds.end()) {
			Core::Logger::errorLog("AudioManager: brak efektu w cache: " + id);
			return false;
		}

		Sound& sound = sound_iterator->second.sound;
		SetSoundVolume(sound, clampVolume(options.volume) * _effects_volume);
		SetSoundPitch(sound, std::max(0.01f, options.pitch));

		if (IsSoundPlaying(sound)) {
			if (!options.restart_if_playing)
				return true;
			StopSound(sound);
		}

		PlaySound(sound);
		return true;
	}

	bool AudioManager::playSoundFile(const std::string& id, const std::string& path, SoundOptions options) {
		if (!loadSound(id, path))
			return false;

		return playSound(id, options);
	}

	void AudioManager::stopSound(const std::string& id) {
		const auto sound_iterator = _sounds.find(id);
		if (sound_iterator == _sounds.end())
			return;

		StopSound(sound_iterator->second.sound);
	}

	bool AudioManager::playMusic(const std::string& path, const bool loop, const float volume) {
		_playlist.clear();
		_playlist_order.clear();
		_playlist_order_position = 0;
		_current_music_loop = loop;
		return loadAndPlayMusic(path, volume);
	}

	bool AudioManager::playPlaylist(const std::vector<std::string>& tracks, const PlaylistOptions options) {
		if (!_is_initialized || tracks.empty())
			return false;

		_playlist = tracks;
		_playlist_options = options;
		_playlist_order_position = 0;
		_current_music_loop = false;
		preparePlaylistOrder();

		if (_playlist_order.empty())
			return false;

		return loadAndPlayMusic(_playlist[_playlist_order[_playlist_order_position]], _playlist_options.volume);
	}

	void AudioManager::stopMusic() {
		_playlist.clear();
		_playlist_order.clear();
		_playlist_order_position = 0;
		unloadCurrentMusic();
	}

	void AudioManager::pauseMusic() {
		if (_is_initialized && _has_music)
			PauseMusicStream(_current_music);
	}

	void AudioManager::resumeMusic() {
		if (_is_initialized && _has_music)
			ResumeMusicStream(_current_music);
	}

	bool AudioManager::isMusicPlaying() const {
		return _is_initialized && _has_music && IsMusicStreamPlaying(_current_music);
	}

	void AudioManager::setMasterVolume(const float volume) {
		_master_volume = clampVolume(volume);
		if (_is_initialized)
			SetMasterVolume(_master_volume);
	}

	void AudioManager::setMusicVolume(const float volume) {
		_music_volume = clampVolume(volume);
		updateMusicVolume();
	}

	void AudioManager::setEffectsVolume(const float volume) {
		_effects_volume = clampVolume(volume);
	}

	bool AudioManager::loadAndPlayMusic(const std::string& path, const float volume) {
		if (!_is_initialized || path.empty())
			return false;

		unloadCurrentMusic();

		_current_music = LoadMusicStream(path.c_str());
		if (_current_music.ctxData == nullptr) {
			Core::Logger::errorLog("AudioManager: nie udalo sie zaladowac muzyki: " + path);
			return false;
		}

		_current_music.looping = false;
		_current_music_path = path;
		_current_music_volume = clampVolume(volume);
		_has_music = true;
		updateMusicVolume();
		PlayMusicStream(_current_music);
		return true;
	}

	void AudioManager::unloadCurrentMusic() {
		if (!_has_music)
			return;

		StopMusicStream(_current_music);
		UnloadMusicStream(_current_music);
		_current_music = {};
		_current_music_path.clear();
		_has_music = false;
	}

	void AudioManager::advancePlaylist() {
		if (_playlist.empty() || _playlist_order.empty()) {
			unloadCurrentMusic();
			return;
		}

		++_playlist_order_position;
		if (_playlist_order_position >= _playlist_order.size()) {
			if (!_playlist_options.loop) {
				stopMusic();
				return;
			}

			_playlist_order_position = 0;
			preparePlaylistOrder();
		}

		loadAndPlayMusic(_playlist[_playlist_order[_playlist_order_position]], _playlist_options.volume);
	}

	void AudioManager::preparePlaylistOrder() {
		_playlist_order.resize(_playlist.size());
		std::iota(_playlist_order.begin(), _playlist_order.end(), 0);

		if (_playlist_options.mode == PlaylistMode::Random && _playlist_order.size() > 1)
			std::shuffle(_playlist_order.begin(), _playlist_order.end(), _random_engine);
	}

	void AudioManager::updateMusicVolume() const {
		if (_is_initialized && _has_music)
			SetMusicVolume(_current_music, _music_volume * _current_music_volume);
	}

	float AudioManager::clampVolume(const float volume) {
		return std::clamp(volume, 0.0f, 1.0f);
	}

} // namespace Nawia::Audio
