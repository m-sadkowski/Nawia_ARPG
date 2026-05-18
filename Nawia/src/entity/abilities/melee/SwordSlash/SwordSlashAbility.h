#pragma once

#include <Ability.h>

#include <raylib.h>

#include <memory>

namespace Nawia::Entity {

	/**
	 * @class SwordSlashAbility
	 * @brief Umiejętność walki wręcz tworząca opóźniony efekt cięcia.
	 */
	class SwordSlashAbility : public Ability {
	public:
		/**
		 * @brief Tworzy cięcie z teksturą efektu i ikoną umiejętności.
		 */
		SwordSlashAbility(const std::shared_ptr<Texture2D>& slash_tex, const std::shared_ptr<Texture2D>& icon_tex);

		/**
		 * @brief Aktualizuje opóźnienie utworzenia efektu po rozpoczęciu ataku.
		 */
		void update(float dt) override;

		/**
		 * @brief Rozpoczyna animację ataku i przygotowuje opóźnione cięcie.
		 */
		AbilitySpawn cast(float target_x, float target_y) override;

	private:
		/** @brief Wylicza moment pojawienia się hitboxa na podstawie animacji. */
		[[nodiscard]] float calculateSpawnDelay() const;
		[[nodiscard]] float calculateAnimationDuration() const;

    /** @brief Dodaje efekt cięcia do pending spawnów źródła użycia. */
		void spawnSlashEffect();

		std::shared_ptr<Texture2D> _slash_tex;
		bool _is_active = false;
		bool _has_spawned = false;
		float _active_time = 0.0f;
		float _spawn_delay = 0.0f;
	};

} // namespace Nawia::Entity
