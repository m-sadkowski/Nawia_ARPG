#include "DialogueManager.h"

#include <Cat.h>
#include <Dialogue.h>
#include <Engine.h>

namespace Nawia::Game {

	void DialogueManager::createCatDialogue(Core::Engine* engine, Entity::Cat* cat) {
		if (!engine || !cat)
			return;

		DialogueTree tree;
		DialogueNode start_node;
		start_node.id = 0;
		start_node.speaker_name = "Kot Olga";
		start_node.text = "Miau. Bandyci ukradli moje ryby i schowali je w skrzyni. Mam klucz, ale sam jej nie otworze.";

		DialogueOption open_inventory_option;
		open_inventory_option.text = "Jasne!";
		open_inventory_option.next_node_id = -1;
		open_inventory_option.action = [engine, cat]() {
			if (!engine || !cat)
				return;

			engine->getUIHandler().closeDialogue();
			engine->getUIHandler().openContainer(cat);
		};

		DialogueOption exit_option;
		exit_option.text = "Nie?";
		exit_option.next_node_id = -1;

		start_node.options.push_back(open_inventory_option);
		start_node.options.push_back(exit_option);

		tree.addNode(start_node);
		cat->setDialogue(tree);
	}

	void DialogueManager::createCatQuestCompletedDialogue(Core::Engine* engine, Entity::Cat* cat) {
		if (!engine || !cat)
			return;

		DialogueNode thank_node;
		thank_node.id = 0;
		thank_node.speaker_name = "Kot Olga";
		thank_node.text = "Miau! To moja ryba. Dzieki, ze zabrales ja bandytom. Masz tu prezent.";

		DialogueOption close_option;
		close_option.text = "Pokaz prezent.";
		close_option.next_node_id = -1;
		close_option.action = [engine, cat]() {
			if (!engine || !cat)
				return;

			engine->getUIHandler().closeDialogue();
			engine->getUIHandler().openContainer(cat);
		};

		DialogueOption exit_option;
		exit_option.text = "Na nic mi twoje prezenty, bywaj.";
		exit_option.next_node_id = -1;

		thank_node.options.push_back(close_option);
		thank_node.options.push_back(exit_option);

		DialogueTree tree;
		tree.addNode(thank_node);
		cat->setDialogue(tree);
	}

} // namespace Nawia::Game
