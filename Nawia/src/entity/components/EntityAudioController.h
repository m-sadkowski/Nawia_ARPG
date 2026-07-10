#pragma once

#include <memory>
#include <string>

namespace Nawia::Audio {
	class AudioManager;
}

namespace Nawia::Entity {
	class Entity;

	class EntityAudioController {
	public:
		static void setListener(const std::shared_ptr<Entity>& listener);

		void playSoundEffect(
			const Entity& owner,
			Audio::AudioManager* audio_manager,
			const std::string& id,
			float volume,
			bool restart_if_playing,
			float pitch) const;
		void stopSoundEffect(Audio::AudioManager* audio_manager, const std::string& id) const;
		void updateMovementSound(
			const Entity& owner,
			Audio::AudioManager* audio_manager,
			const std::string& path,
			bool should_play,
			float volume,
			float pitch);
		void stopMovementSound(Audio::AudioManager* audio_manager) const;

	private:
		[[nodiscard]] static float getSpatialVolumeMultiplier(const Entity& owner);

		std::string _movement_sound_id;
	};

}
