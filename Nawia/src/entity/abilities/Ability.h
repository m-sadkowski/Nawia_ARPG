#pragma once
#include <AbilityEffect.h>
#include <AbilityStats.h>

#include <raylib.h>
#include <functional>
#include <vector>
#include <string>
#include <memory>

namespace Nawia::Entity
{
	class Entity;
	
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
	 * @see AbilityEffect Efekty tworzone przez umiejętności.
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
		Ability(std::string name, const AbilityStats& stats, AbilityTargetType target_type, const std::shared_ptr<Texture2D>& icon_texture);

		virtual ~Ability() = default;

		/**
	 * @brief Aktualizuje stan umiejętności, głównie czas odnowienia.
		 * @param dt Czas od poprzedniej klatki w sekundach.
		 */
		virtual void update(float dt);

		/**
		 * @brief Sprawdza, czy umiejętność jest gotowa do użycia.
	 * @return `true`, jeśli czas odnowienia dobiegł końca.
		 */
		[[nodiscard]] bool isReady() const;
		
		/**
		 * @brief Używa umiejętności we wskazanym punkcie świata.
		 *
	 * Implementacje zwracają encję do utworzenia albo `nullptr`, jeśli efekt
		 * jest natychmiastowy lub opóźniony.
		 *
		 * @param target_x Współrzędna X celu.
		 * @param target_y Współrzędna Y celu, mapowana na Z świata 3D.
	 * @return Encja do utworzenia albo `nullptr`.
		 */
		virtual std::unique_ptr<Entity> cast(float target_x, float target_y) = 0;
		
		[[nodiscard]] std::string getName() const;
		[[nodiscard]] float getCastRange() const;
		[[nodiscard]] float getCooldownTimer() const { return _cooldown_timer; }
		[[nodiscard]] AbilityTargetType getTargetType() const;
		[[nodiscard]] const AbilityStats& getStats() const { return _stats; }
		

		[[nodiscard]] std::shared_ptr<Texture2D> getIcon() const { return _icon_texture; }
		
		void setCaster(Entity* caster) { _caster = caster; }
		[[nodiscard]] Entity* getCaster() const { return _caster; }

	protected:
		std::string _name;
		std::shared_ptr<Texture2D> _icon_texture;
		AbilityStats _stats;
		float _cooldown_timer;
		AbilityTargetType _target_type;
	Entity* _caster;  ///< Encja posiadająca tę umiejętność.

	/// Uruchamia czas odnowienia po udanym użyciu.
		void startCooldown() { _cooldown_timer = _stats.cooldown; }
	};

} // namespace Nawia::Entity
