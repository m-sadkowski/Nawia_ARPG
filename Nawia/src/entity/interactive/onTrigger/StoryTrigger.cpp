#include "StoryTrigger.h"

#include "StoryTriggerInternal.h"

#include <Collider.h>
#include <Engine.h>
#include <StoryConditions.h>
#include <UIHandler.h>

#include <utility>

namespace Nawia::Entity {

	StoryTrigger::StoryTrigger(
		const std::string& name,
		const float x,
		const float y,
		const float width,
		const float height,
		Core::Engine* engine,
		nlohmann::json data
	)
		: InteractiveTrigger(name, x, y, nullptr, 1),
		  _engine(engine),
		  _data(std::move(data))
	{
		setType(EntityType::Trigger);
		setFaction(Faction::None);
		_once = _data.value("once", true);
		setCollider(std::make_unique<RectangleCollider>(this, width, height, 0.0f, 0.0f));
	}

	void StoryTrigger::onTriggerEnter(Entity& other) {
		if (isDormant() || _dialogue_open)
			return;

		if (_once && _completed)
			return;

		if (other.getFaction() != Faction::Player)
			return;

		if (!Game::areEntityConditionsMet(_data, _engine))
			return;

		for (const auto& action : StoryTriggerSupport::collectEnterActions(_data))
			StoryTriggerSupport::executeActionData(_engine, this, action);

		run(_engine);
	}

	void StoryTrigger::run(Core::Engine* engine) {
		if (!engine)
			return;

		engine->cancelPlayerAction();

		const nlohmann::json dialogue_json = StoryTriggerSupport::resolveDialogueJson(_data);
		if (!dialogue_json.is_object()) {
			executeActions(engine);
			return;
		}

		Game::DialogueTree tree = StoryTriggerSupport::buildDialogueTree(dialogue_json, [this, engine](const nlohmann::json& action) {
			StoryTriggerSupport::executeActionData(engine, this, action);
		});
		if (!tree.getNode(0)) {
			executeActions(engine);
			return;
		}

		_dialogue_open = true;
		engine->getUIHandler().openDialogueFacing(
			tree,
			StoryTriggerSupport::resolveDialogueSpeaker(engine, _data),
			0,
			[this, engine](const int, const bool completed) {
				_dialogue_open = false;
				if (!completed)
					return;

				executeActions(engine);
			}
		);
	}

	void StoryTrigger::executeActions(Core::Engine* engine) {
		if (!engine)
			return;

		for (const auto& action : StoryTriggerSupport::collectActions(_data))
			StoryTriggerSupport::executeActionData(engine, this, action);

		_completed = true;
		if (_once)
			setDormant(true);
	}

	void StoryTrigger::render(const Camera3D& camera) {
		if (DebugColliders) {
			auto* rect_collider = dynamic_cast<RectangleCollider*>(getCollider());
			if (!rect_collider)
				return;

			const Vector2 center = rect_collider->getPosition();
			const float width = rect_collider->getWidth();
			const float height = rect_collider->getHeight();
			DrawCubeWires(Vector3{center.x, getAltitude() + 0.1f, center.y}, width, 0.2f, height, SKYBLUE);

			const Vector2 screen_pos = GetWorldToScreen(Vector3{center.x, getAltitude() + 0.6f, center.y}, camera);
			DrawText(getName().c_str(), static_cast<int>(screen_pos.x - 35), static_cast<int>(screen_pos.y - 10), 10, SKYBLUE);
		}
	}

	float StoryTrigger::getInteractionRange() {
		return 0.0f;
	}

	nlohmann::json StoryTrigger::serializeState() const {
		nlohmann::json state = Entity::serializeState();
		state["completed"] = _completed;
		state["once"] = _once;
		return state;
	}

	void StoryTrigger::applyState(const nlohmann::json& state, Item::ItemDatabase* item_database) {
		Entity::applyState(state, item_database);
		if (!state.is_object())
			return;

		_completed = state.value("completed", _completed);
		_once = state.value("once", _once);
		if (_completed && _once)
			setDormant(true);
	}

} // namespace Nawia::Entity
