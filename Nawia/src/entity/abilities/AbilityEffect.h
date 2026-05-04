#pragma once

#include <Entity.h>
#include <AbilityStats.h>

#include <vector>

namespace Nawia::Entity {

	/**
	 * @class AbilityEffect
	 * @brief Encja efektu tworzona przez umiejętność.
	 *
	 * Efekt dodaje do `Entity` czas życia, obsługę kolizji bojowych oraz listę
	 * trafionych celów, żeby ten sam efekt nie zadawał obrażeń wielokrotnie.
	 *
	 * @see Ability Umiejętność tworząca efekt.
	 */
	class AbilityEffect : public Entity {
	public:
		/**
		 * @brief Tworzy efekt umiejętności w podanym punkcie świata.
		 * @param name Nazwa efektu.
		 * @param x Początkowa pozycja X.
		 * @param y Początkowa pozycja Y, mapowana na Z świata 3D.
		 * @param tex Tekstura efektu.
		 * @param stats Statystyki obrażeń, czasu życia i hitboxa.
		 */
		AbilityEffect(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& tex, const AbilityStats& stats);

		/**
		 * @brief Aktualizuje ruch i czas życia efektu.
		 */
		void update(float dt) override;
		
		/**
		 * @brief Sprawdza, czy efekt przekroczył swój czas życia.
		 * @return `true`, jeśli efekt powinien zostać usunięty.
		 */
		[[nodiscard]] bool isExpired() const;
		
		[[nodiscard]] int getDamage() const;
		[[nodiscard]] const AbilityStats& getStats() const { return _stats; }

		/**
		 * @brief Sprawdza kolizję efektu z celem.
		 * @param target Encja sprawdzana pod kątem trafienia.
		 * @return `true`, jeśli wykryto kolizję.
		 */
		[[nodiscard]] virtual bool checkCollision(const std::shared_ptr<Entity>& target) const;
		
		/**
		 * @brief Wywoływane po wykryciu trafienia.
		 * @param target Encja trafiona przez efekt.
		 */
		virtual void onCollision(const std::shared_ptr<Entity>& target);
		
		/**
		 * @brief Sprawdza, czy cel został już trafiony przez ten efekt.
		 */
		[[nodiscard]] bool hasHit(const std::shared_ptr<Entity>& target) const;
		
		/// Zapisuje cel jako już trafiony.
		void addHit(const std::shared_ptr<Entity>& target);

	protected:
		AbilityStats _stats;
		float _timer;  ///< Licznik czasu życia efektu.
		std::vector<std::weak_ptr<Entity>> _hit_entities;  ///< Encje już trafione przez efekt.
	};

} // namespace Nawia::Entity
