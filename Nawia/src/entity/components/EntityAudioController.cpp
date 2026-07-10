#include "EntityAudioController.h"

#include <AudioManager.h>
#include <Entity.h>

#include <raymath.h>

#include <cstdint>

namespace Nawia::Entity {
namespace {
	std::weak_ptr<Entity> g_audio_listener;

	constexpr float FULL_VOLUME_DISTANCE = 5.0f;
	constexpr float MEDIUM_VOLUME_DISTANCE = 10.0f;
	constexpr float LOW_VOLUME_DISTANCE = 15.0f;
}

	void EntityAudioController::setListener(const std::shared_ptr<Entity>& listener)
	{
		g_audio_listener = listener;
	}

	void EntityAudioController::playSoundEffect(
		const Entity& owner,
		Audio::AudioManager* audio_manager,
		const std::string& id,
		const float volume,
		const bool restart_if_playing,
		const float pitch) const
	{
		if (!audio_manager)
			return;

		const float spatial_volume = getSpatialVolumeMultiplier(owner);
		if (spatial_volume <= 0.0f)
			return;

		audio_manager->playSound(id, Audio::SoundOptions{volume * spatial_volume, pitch, restart_if_playing});
	}

	void EntityAudioController::stopSoundEffect(Audio::AudioManager* audio_manager, const std::string& id) const
	{
		if (!audio_manager)
			return;

		audio_manager->stopSound(id);
	}

	void EntityAudioController::updateMovementSound(
		const Entity& owner,
		Audio::AudioManager* audio_manager,
		const std::string& path,
		const bool should_play,
		const float volume,
		const float pitch)
	{
		if (!audio_manager)
			return;

		if (_movement_sound_id.empty())
			_movement_sound_id = "movement:" + std::to_string(reinterpret_cast<std::uintptr_t>(&owner));

		const float spatial_volume = getSpatialVolumeMultiplier(owner);
		if (should_play && spatial_volume > 0.0f)
			audio_manager->playSoundFile(_movement_sound_id, path, Audio::SoundOptions{volume * spatial_volume, pitch, false});
		else
			audio_manager->stopSound(_movement_sound_id);
	}

	void EntityAudioController::stopMovementSound(Audio::AudioManager* audio_manager) const
	{
		if (!audio_manager || _movement_sound_id.empty())
			return;

		audio_manager->stopSound(_movement_sound_id);
	}

	float EntityAudioController::getSpatialVolumeMultiplier(const Entity& owner)
	{
		const auto listener = g_audio_listener.lock();
		if (!listener || listener.get() == &owner)
			return 1.0f;

		const float distance = Vector2Distance(owner.getCenter(), listener->getCenter());
		if (distance <= FULL_VOLUME_DISTANCE)
			return 1.0f;
		if (distance <= MEDIUM_VOLUME_DISTANCE)
			return 2.0f / 3.0f;
		if (distance <= LOW_VOLUME_DISTANCE)
			return 1.0f / 3.0f;
		return 0.0f;
	}

}
