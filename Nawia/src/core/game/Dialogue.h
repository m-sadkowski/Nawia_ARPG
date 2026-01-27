#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace Nawia::Game {

    struct DialogueOption {
        std::string text;
        int nextNodeID; // -1 = end of dialogue

        std::function<void()> action = nullptr;
    };

    struct DialogueNode {
        int id;
        std::string text;
        std::string speakerName;
        std::vector<DialogueOption> options;
    };

    class DialogueTree {
    public:
        void addNode(const DialogueNode& node) {
            _nodes[node.id] = node;
        }

        const DialogueNode* getNode(int id) const {
            if (_nodes.find(id) != _nodes.end()) {
                return &_nodes.at(id);
            }
            return nullptr;
        }

    private:
        std::map<int, DialogueNode> _nodes;
    };
}