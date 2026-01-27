#include "DialogueManager.h"
#include "Dialogue.h"
#include "Engine.h"

namespace Nawia::Game {

    void DialogueManager::createCatDialogue(Core::Engine* engine, std::shared_ptr<Entity::Cat> cat) {
        Game::DialogueTree tree;
        Game::DialogueNode start_node;
        start_node.id = 0;
        start_node.speakerName = "Kot \"Olga\"";
        start_node.text = "Jestem tak jak mewa - przylatuje i zabieram!";

        Game::DialogueOption open_eq_opt;
        open_eq_opt.text = "FEIN FEIN FEIN";
        open_eq_opt.nextNodeID = -1;
        open_eq_opt.action = [engine, cat]() {
            engine->getUIHandler().closeDialogue();
            engine->getUIHandler().openContainer(cat);
            };

        DialogueOption exit_option;
        exit_option.text = "wthelly";
        exit_option.nextNodeID = -1;

        start_node.options.push_back(open_eq_opt);
        start_node.options.push_back(exit_option);

        tree.addNode(start_node);
        cat->setDialogue(tree);
    }

}