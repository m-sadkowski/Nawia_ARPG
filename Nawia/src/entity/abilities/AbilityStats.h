#pragma once

namespace Nawia::Entity {

	/**
	 * @struct AbilityStats
	 * @brief Zestaw parametrów liczbowych używanych przez umiejętności i ich efekty.
	 */
	struct AbilityStats {
		float cooldown = 0.0f;          ///< Czas odnowienia po użyciu.
		float duration = 0.0f;          ///< Czas życia efektu lub aktywnego stanu.
		float cast_range = 0.0f;        ///< Maksymalny zasięg użycia.
		float projectile_speed = 0.0f;  ///< Prędkość pocisku, jeśli umiejętność go tworzy.
		int damage = 0;                 ///< Bazowe obrażenia umiejętności.
		float hitbox_radius = 0.0f;     ///< Promień lub zasięg wolumenu trafienia.
	};

} // namespace Nawia::Entity
