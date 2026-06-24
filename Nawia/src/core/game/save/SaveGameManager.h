#pragma once

#include <json.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace Nawia::Core {
	class Engine;
}

namespace Nawia::Game {

	/**
	 * @struct SaveSlotInfo
	 * @brief Metadane slotu zapisu prezentowane w menu wczytywania.
	 */
	struct SaveSlotInfo {
		int slot = 0;
		bool occupied = false;
		std::string saved_at;
		std::string current_level;
		std::string current_location;
	};

	/**
	 * @class SaveGameManager
	 * @brief Zarzadza slotami zapisu i przeplywem save/load dla silnika.
	 *
	 * Pojedynczy slot trzymamy w katalogu `saves/slot_N/` z plikiem `save.json`.
	 * Klasa odpowiada wylacznie za scalanie/rozdzielanie stanu z managerow gry;
	 * faktyczna serializacja danych nalezy do ich wlascicieli (Player, Quest-,
	 * Boss-, SpawnManager itd.).
	 */
	class SaveGameManager {
	public:
		/** @brief Zwraca, czy istnieje przynajmniej jeden uzywany slot. */
		[[nodiscard]] bool hasAnySave() const;

		/** @brief Zwraca slot, z ktorego ostatnio wczytano lub zapisano gre. */
		[[nodiscard]] int getActiveSlot() const { return _active_slot; }

		/** @brief Zeruje informacje o aktywnym slocie (np. po starcie nowej gry). */
		void clearActiveSlot() { _active_slot = 0; }

		/** @brief Zwraca metadane wszystkich slotow, w tym pustych. */
		[[nodiscard]] std::vector<SaveSlotInfo> getSaveSlots() const;

		/** @brief Zapisuje stan silnika do wybranego slotu. */
		bool saveGame(Core::Engine& engine, int slot);

		/**
		 * @brief Wczytuje stan silnika z wybranego slotu.
		 * @param slot Numer slotu (1..N) albo 0, aby uzyc najnowszego zapisu.
		 */
		bool loadGame(Core::Engine& engine, int slot);

		/**
		 * @brief Odczytuje JSON zapisu bez ladowania poziomu (do ekranu ladowania).
		 */
		bool tryReadSave(int slot, nlohmann::json& out_data, int& resolved_slot) const;

		/**
		 * @brief Stosuje stan zapisu po zaladowaniu poziomu i zasobow.
		 */
		void applySaveState(Core::Engine& engine, const nlohmann::json& save_state, int slot);

	private:
		[[nodiscard]] static std::filesystem::path getSaveRoot();
		[[nodiscard]] static std::filesystem::path getSlotPath(int slot);
		[[nodiscard]] static std::filesystem::path getSlotFilePath(int slot);
		[[nodiscard]] static bool isValidSlot(int slot);
		[[nodiscard]] int findLatestSlot() const;

		int _active_slot = 0;
	};

} // namespace Nawia::Game
