#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Nawia::Game {

	/**
	 * @struct DialogueOption
	 * @brief Opcja wyboru widoczna w oknie dialogu.
	 */
	struct DialogueOption {
		std::string text;
		int next_node_id = -1; ///< -1 konczy dialog.
		std::function<void()> action = nullptr;
	};

	/**
	 * @struct DialogueNode
	 * @brief Pojedynczy wezel dialogu z tekstem i opcjami odpowiedzi.
	 */
	struct DialogueNode {
		int id = 0;
		std::string text;
		std::string speaker_name;
		std::vector<DialogueOption> options;
	};

	/**
	 * @class DialogueTree
	 * @brief Przechowuje wezly dialogu indeksowane po ID.
	 */
	class DialogueTree {
	public:
		/**
		 * @brief Dodaje albo podmienia wezel dialogu.
		 */
		void addNode(const DialogueNode& node) { _nodes[node.id] = node; }

		/**
		 * @brief Zwraca wezel po ID albo nullptr.
		 */
		[[nodiscard]] const DialogueNode* getNode(int id) const {
			if (_nodes.contains(id))
				return &_nodes.at(id);

			return nullptr;
		}

	private:
		std::map<int, DialogueNode> _nodes;
	};

} // namespace Nawia::Game
