#pragma once

#include <AbilityStats.h>

#include <raylib.h>

#include <memory>
#include <string>

namespace Nawia::Entity {

	class Entity;

	/**
	 * @brief Typ encji zwracanej przez umiejętność do dodania do świata.
	 */
	using AbilitySpawn = std::shared_ptr<Entity>;

	/**
	 * @enum AbilityTargetType
	 * @brief Określa sposób wybierania celu przez umiejętność.
	 */
	enum class AbilityTargetType {
		POINT, ///< Celowanie w punkt świata, np. fireball.
		UNIT,  ///< Celowanie w konkretną encję.
		SELF   ///< Celowanie w źródło użycia, np. leczenie albo tarcza.
	};

	/**
	 * @class Ability
	 * @brief Bazowa klasa wszystkich umiejętności w grze.
	 *
	 * Umiejętność jest obiektem logicznym przypiętym do źródła użycia. Nie renderuje
	 * się sama, tylko tworzy efekty, pociski albo inne encje po użyciu.
	 *
	 * @see AbilityStats Konfiguracja obrażeń, czasu odnowienia i zasięgu.
	 */
	class Ability {
	public:
		/**
		 * @brief Tworzy umiejętność z nazwą, statystykami i ikoną.
		 * @param name Nazwa umiejętności.
		 * @param stats Konfiguracja obrażeń, czasu odnowienia i zasięgu.
		 * @param target_type Sposób wybierania celu.
		 * @param icon_texture Ikona pokazywana w UI.
		 */
		Ability(std::string name,
				const AbilityStats& stats,
				AbilityTargetType target_type,
				const std::shared_ptr<Texture2D>& icon_texture);

		virtual ~Ability() = default;

		/**
		 * @brief Aktualizuje stan umiejętności, głównie czas odnowienia.
		 * @param dt Czas od poprzedniej klatki w sekundach.
		 */
		virtual void update(float dt);

		/** @brief Przerywa aktywne, opoznione uzycie ability bez resetowania cooldownu. */
		virtual void cancel() {}

		/**
		 * @brief Sprawdza, czy umiejętność jest gotowa do użycia.
		 * @return `true`, jeśli czas odnowienia dobiegł końca.
		 */
		[[nodiscard]] bool isReady() const;

		/**
		 * @brief Sprawdza, czy umiejętność ma źródło użycia i może rozpocząć cast.
		 */
		[[nodiscard]] bool canCast() const;

		/**
		 * @brief Używa umiejętności we wskazanym punkcie świata.
		 *
		 * Implementacje zwracają encję do dodania do świata albo `nullptr`, jeśli
     * efekt jest natychmiastowy lub zostanie utworzony z opóźnieniem przez źródło użycia.
		 *
		 * @param target_x Współrzędna X celu.
		 * @param target_y Współrzędna Y celu, mapowana na Z świata 3D.
		 * @return Encja do dodania do świata albo `nullptr`.
		 */
		virtual AbilitySpawn cast(float target_x, float target_y) = 0;

		/** @brief Zwraca nazwę umiejętności. */
		[[nodiscard]] const std::string& getName() const { return _name; }

		/** @brief Zwraca zasięg użycia umiejętności. */
		[[nodiscard]] float getCastRange() const { return _stats.cast_range; }

		/** @brief Zwraca pozostały czas odnowienia w sekundach. */
		[[nodiscard]] float getCooldownTimer() const { return _cooldown_timer; }

    /** @brief Zwraca wypełnienie czasu odnowienia w zakresie 0-1. */
		[[nodiscard]] float getCooldownRatio() const;

		/** @brief Zwraca sposób wybierania celu przez umiejętność. */
		[[nodiscard]] AbilityTargetType getTargetType() const { return _target_type; }

		/** @brief Zwraca statystyki konfiguracyjne umiejętności. */
		[[nodiscard]] const AbilityStats& getStats() const { return _stats; }

		/** @brief Zwraca ikonę umiejętności wyświetlaną w UI. */
		[[nodiscard]] std::shared_ptr<Texture2D> getIcon() const { return _icon_texture; }

		/** @brief Ustawia encję, która posiada i używa tej umiejętności. */
		void setCaster(Entity* caster) { _caster = caster; }

		/** @brief Zwraca encję posiadającą tę umiejętność. */
		[[nodiscard]] Entity* getCaster() const { return _caster; }

	protected:
		/**
     * @brief Rozpoczyna użycie i uruchamia czas odnowienia, jeśli umiejętność jest gotowa.
		 */
		bool beginCast();

		/** @brief Uruchamia czas odnowienia po udanym użyciu. */
		void startCooldown();

		std::string _name;
		std::shared_ptr<Texture2D> _icon_texture;
		AbilityStats _stats;
		float _cooldown_timer = 0.0f;
		AbilityTargetType _target_type = AbilityTargetType::POINT;
		Entity* _caster = nullptr;
	};

} // namespace Nawia::Entity
