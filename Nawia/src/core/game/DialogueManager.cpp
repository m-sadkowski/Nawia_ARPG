#include "DialogueManager.h"
#include "Dialogue.h"
#include "Engine.h"

namespace Nawia::Game {

    void DialogueManager::createCatDialogue(Core::Engine* engine, const std::shared_ptr<Entity::Cat>& cat) 
	{
        Game::DialogueTree tree;
        Game::DialogueNode start_node;
        start_node.id = 0;
        start_node.speaker_name = "David";
        start_node.text = "Can you bring me some food from chest?";

        Game::DialogueOption open_eq_opt;
        open_eq_opt.text = "Sure!";
        open_eq_opt.next_node_id = -1;
        open_eq_opt.action = [engine, cat]() {
            engine->getUIHandler().closeDialogue();
            engine->getUIHandler().openContainer(cat);
            };

        DialogueOption exit_option;
        exit_option.text = "No?";
        exit_option.next_node_id = -1;

        start_node.options.push_back(open_eq_opt);
        start_node.options.push_back(exit_option);

        tree.addNode(start_node);
        cat->setDialogue(tree);
    }

}