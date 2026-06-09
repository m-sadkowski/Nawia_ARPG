#include "DialogueManager.h"

#include <Cat.h>
#include <Dialogue.h>
#include <Engine.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace Nawia::Game {
	namespace {
		constexpr const char* DIALOGUE_PLACEHOLDER_VOICE = "assets/audio/dialogues/Placeholders/dialogue_placeholder.wav";

		DialogueOption makeOption(std::string text, const int next_node_id, std::function<void()> action = nullptr)
		{
			DialogueOption option;
			option.text = std::move(text);
			option.next_node_id = next_node_id;
			option.action = std::move(action);
			return option;
		}

		DialogueNode makeNode(
			const int id,
			std::string speaker,
			std::string text,
			std::vector<DialogueOption> options)
		{
			DialogueNode node;
			node.id = id;
			node.speaker_name = std::move(speaker);
			node.text = std::move(text);
			node.voice_path = DIALOGUE_PLACEHOLDER_VOICE;
			node.options = std::move(options);
			return node;
		}
	}

	void DialogueManager::createCatDialogue(Core::Engine* engine, Entity::Cat* cat) {
		if (!engine || !cat)
			return;

		DialogueTree tree;
		tree.addNode(makeNode(0, "Kot Olga",
			"Miau. Bandyci ukradli moje ryby i schowali je w skrzyni. Mam klucz, ale sam jej nie otworze.",
			{
				makeOption("Jasne!", 1),
				makeOption("Nie?", 2)
			}));
		tree.addNode(makeNode(1, "Logos", "Jasne!", {
			makeOption("Dalej.", -1, [engine, cat]() {
				if (!engine || !cat)
					return;

				engine->getUIHandler().closeDialogue();
				engine->getUIHandler().openContainer(cat);
			})
		}));
		tree.addNode(makeNode(2, "Logos", "Nie?", {
			makeOption("Dalej.", -1)
		}));
		cat->setDialogue(tree);
	}

	void DialogueManager::createCatQuestCompletedDialogue(Core::Engine* engine, Entity::Cat* cat) {
		if (!engine || !cat)
			return;

		DialogueTree tree;
		tree.addNode(makeNode(0, "Kot Olga",
			"Miau! To moja ryba. Dzieki, ze zabrales ja bandytom. Masz tu prezent.",
			{
				makeOption("Pokaz prezent.", 1),
				makeOption("Na nic mi twoje prezenty, bywaj.", 2)
			}));
		tree.addNode(makeNode(1, "Logos", "Pokaz prezent.", {
			makeOption("Dalej.", -1, [engine, cat]() {
				if (!engine || !cat)
					return;

				engine->getUIHandler().closeDialogue();
				engine->getUIHandler().openContainer(cat);
			})
		}));
		tree.addNode(makeNode(2, "Logos", "Na nic mi twoje prezenty, bywaj.", {
			makeOption("Dalej.", -1)
		}));
		cat->setDialogue(tree);
	}

} // namespace Nawia::Game
