#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace Nawia::Game {

    struct DialogueOption {
        std::string text;
        int next_node_id; // -1 = end of dialogue

        std::function<void()> action = nullptr;
    };

    struct DialogueNode {
        int id;
        std::string text;
        std::string speaker_name;
        std::vector<DialogueOption> options;
    };

    class DialogueTree {
    public:
        void addNode(const DialogueNode& node) { _nodes[node.id] = node; }

        const DialogueNode* getNode(const int id) const {
            if (_nodes.contains(id))
                return &_nodes.at(id);
            
            return nullptr;
        }

    private:
        std::map<int, DialogueNode> _nodes;
    };

} // namespace Nawia::Game