#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Nawia::Core {
	class Engine;
}

namespace Nawia::Game {

	struct SaveSlotInfo {
		int slot = 0;
		bool occupied = false;
		std::string saved_at;
		std::string current_level;
		std::string current_location;
	};

	class SaveGameManager {
	public:
		[[nodiscard]] bool hasAnySave() const;
		[[nodiscard]] int getActiveSlot() const { return _active_slot; }
		[[nodiscard]] std::vector<SaveSlotInfo> getSaveSlots() const;

		void clearActiveSlot() { _active_slot = 0; }
		bool saveGame(Core::Engine& engine, int slot);
		bool loadLatestGame(Core::Engine& engine);
		bool loadGame(Core::Engine& engine, int slot);

	private:
		[[nodiscard]] std::filesystem::path getSaveRoot() const;
		[[nodiscard]] std::filesystem::path getSlotPath(int slot) const;
		[[nodiscard]] bool isValidSlot(int slot) const;
		[[nodiscard]] int findLatestSlot() const;

		int _active_slot = 0;
	};

} // namespace Nawia::Game
