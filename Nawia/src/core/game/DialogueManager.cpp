#include "DialogueManager.h"
#include "Dialogue.h"
#include "Engine.h"

namespace Nawia::Game {

    void DialogueManager::createCatDialogue(Core::Engine* engine, Entity::Cat* cat) 
	{
        Game::DialogueTree tree;
        Game::DialogueNode start_node;
        start_node.id = 0;
        start_node.speaker_name = "Kot Olga";
        start_node.text = "Miau. Czy mozesz przyniesc odzyskac moja rybe? Ukradla mi ja mewa ktora zjadl diabel - mam klucz do skrzyni diabla.";

        Game::DialogueOption open_eq_opt;
        open_eq_opt.text = "Jasne!";
        open_eq_opt.next_node_id = -1;
        open_eq_opt.action = [engine, cat]() {
            engine->getUIHandler().closeDialogue();
            engine->getUIHandler().openContainer(cat);
        };

        DialogueOption exit_option;
        exit_option.text = "Nie?";
        exit_option.next_node_id = -1;

        start_node.options.push_back(open_eq_opt);
        start_node.options.push_back(exit_option);

        tree.addNode(start_node);
        cat->setDialogue(tree);
    }

    void DialogueManager::createCatQuestCompletedDialogue(Core::Engine* engine, Entity::Cat* cat)
    {
         Game::DialogueNode thank_node;
         thank_node.id = 0;
         thank_node.speaker_name = "Kot Olga";
         thank_node.text = "Miau! Dzieki za rybe! Masz tu prezent.";

         Game::DialogueOption open_eq_opt;
         open_eq_opt.text = "Dzieki!";
         open_eq_opt.next_node_id = -1;
         
         open_eq_opt.action = [engine, cat]() {
            engine->getUIHandler().closeDialogue();
            engine->getUIHandler().openContainer(cat);
         };

         thank_node.options.push_back(open_eq_opt);

         Game::DialogueOption exit_opt;
         exit_opt.text = "Na nic mi twoje prezenty, bywaj.";
         exit_opt.next_node_id = -1;
         thank_node.options.push_back(exit_opt);
         
         Game::DialogueTree tree;
         tree.addNode(thank_node);
         cat->setDialogue(tree);
    }

} // namespace Nawia::Game