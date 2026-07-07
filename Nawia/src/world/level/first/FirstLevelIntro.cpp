#include "FirstLevel.h"

#include "FirstLevelInternal.h"

#include <AudioManager.h>
#include <Engine.h>
#include <EntityManager.h>
#include <Logger.h>
#include <Map.h>
#include <Player.h>
#include <QuestManager.h>
#include <ResourceManager.h>
#include <SzeptuchaNpc.h>
#include <UIHandler.h>
#include <WandaCorpseNpc.h>

#include <algorithm>
#include <cmath>

namespace Nawia::World {

	namespace F = FirstLevelSupport;

	void FirstLevel::startIntroSequence(Core::Engine* engine) {
		if (!engine || !engine->getPlayer())
			return;

		removeIntroNpc();
		spawnIntroCorpse(engine);

		_intro_slides.clear();
		const auto& config = F::introConfig();
		if (config.contains("slides") && config["slides"].is_array()) {
			for (const auto& slide_json : config["slides"]) {
				IntroSlide slide;
				slide.text = slide_json.value("text", "");
				slide.voice_path = slide_json.value("voice_path", "");
				slide.image_path = slide_json.value("image_path", "");
				slide.duration = F::resolveSlideDuration(slide_json, 6.0f);
				_intro_slides.push_back(std::move(slide));
			}
		}

		for (auto& slide : _intro_slides) {
			if (!slide.image_path.empty()) {
				slide.image_texture = engine->getResourceManager().getTexture(slide.image_path);
				if (slide.image_texture && slide.image_texture->id > 0)
					SetTextureFilter(*slide.image_texture, TEXTURE_FILTER_TRILINEAR);
			}
		}

		_intro_slide_index = 0;
		_intro_phase = IntroPhase::Slides;
		_intro_timer = 0.0f;
		_intro_overlay_alpha = 1.0f;
		_intro_dialogue_opened = false;
		_intro_flash_timer = 0.0f;
		if (_intro_slides.empty()) {
			openAwakeningDialogue(engine);
			return;
		}
		playSlideVoice(engine);
		engine->getPlayer()->stop();
	}

	void FirstLevel::skipIntroSlides(Core::Engine* engine) {
		if (!engine || !engine->getPlayer())
			return;

		stopSlideVoice(engine);
		_intro_slides.clear();
		_intro_slide_index = 0;
		_intro_timer = 0.0f;
		_intro_overlay_alpha = 0.38f;
		_intro_flash_timer = 0.0f;
		_pending_szeptucha_encounter = false;
		_pending_final_dialogue = false;
		_pending_corpse_completion = false;
		_pending_szeptucha_delay = 0.0f;
		removeIntroNpc();
		spawnIntroCorpse(engine);
		openAwakeningDialogue(engine);
	}

	void FirstLevel::skipOutroSlides(Core::Engine* engine) {
		if (!engine)
			return;

		stopSlideVoice(engine);
		_intro_slides.clear();
		_intro_slide_index = 0;
		_intro_timer = 0.0f;
		_intro_overlay_alpha = 1.0f;
		_intro_phase = IntroPhase::OutroFadeToMenu;
		engine->requestReturnToMainMenu(F::outroConfig().value("beta_message", "Twierdza Kamienna nie jest dostepna w wersji Beta."));
	}

	void FirstLevel::equipPresentationBoots(Core::Engine* engine) const {
		if (!engine || !engine->getPlayer())
			return;

		if (const auto boots = engine->getItemDatabase().createItem(F::PRESENTATION_BOOTS_ITEM_ID))
			engine->getPlayer()->getBackpack().addItem(boots);
	}

	void FirstLevel::spawnIntroCorpse(Core::Engine* engine) {
		if (!engine || !engine->getPlayer())
			return;

		for (const auto& entity : engine->getEntityManager().getEntities()) {
			const auto corpse = std::dynamic_pointer_cast<Entity::WandaCorpseNpc>(entity);
			if (corpse) {
				corpse->setDormant(false);
				_intro_corpse = corpse;
				return;
			}
		}

		if (const auto corpse = _intro_corpse.lock())
			corpse->setDormant(true);

		const auto player = engine->getPlayer();
		const float angle = static_cast<float>(GetRandomValue(0, 628)) / 100.0f;
		const float radius = static_cast<float>(GetRandomValue(300, 500)) / 100.0f;
		Vector3 corpse_position = {
			player->getCenter().x + std::cos(angle) * radius,
			player->getAltitude(),
			player->getCenter().y + std::sin(angle) * radius
		};

		if (engine->getCurrentMap() && engine->getCurrentMap()->getNavMesh().isReady())
			corpse_position = engine->getCurrentMap()->getNavMesh().getClosestWalkablePosition(corpse_position);

		auto corpse = std::make_shared<Entity::WandaCorpseNpc>("Zwloki Wandy", corpse_position.x, corpse_position.z, engine);
		corpse->setAltitude(corpse_position.y);
		corpse->setAudioManager(&engine->getAudioManager());
		engine->spawnEntity(corpse);
		_intro_corpse = corpse;
	}

	void FirstLevel::openAwakeningDialogue(Core::Engine* engine) {
		if (!engine)
			return;

		auto player = engine->getPlayer();
		if (player) {
			player->stop();
			const int death_frames = player->getAnimationFrameCount("Death01");
			player->setAnimationSpeed(0.0f);
			player->playAnimation("Death01", false, true, std::max(0, death_frames - 1), true);
		}

		Game::DialogueTree tree = F::buildSingleNodeDialogueTreeFromJson(F::introConfig()["awakening_dialogue"]);

		_intro_dialogue_opened = true;
		_intro_phase = IntroPhase::AwakeningDialogue;
		_intro_timer = 0.0f;
		engine->getUIHandler().openDialogue(tree, 0, [this, engine](const int, const bool) {
			if (const auto player = engine->getPlayer()) {
				player->setAnimationSpeed(Entity::Player::DEFAULT_ANIMATION_SPEED);
				player->playAnimation("LayToIdle", false, true, 0, true);
			}
			if (engine->getQuestManager().startQuest("inspect_wanda_body"))
				engine->getUIHandler().showNotification("Nowy quest: Sprawdz zwloki kobiety", 4.0f);
			_intro_phase = IntroPhase::InspectCorpse;
			_intro_timer = 0.0f;
			_intro_dialogue_opened = false;
		});
	}

	void FirstLevel::queueCorpseInspected(const Vector2& corpse_position) {
		_pending_corpse_completion = true;
		queueSzeptuchaEncounter(corpse_position);
	}

	void FirstLevel::queueSzeptuchaEncounter(const Vector2& corpse_position) {
		_pending_szeptucha_encounter = true;
		_pending_szeptucha_position = corpse_position;
		_pending_szeptucha_delay = 0.05f;
	}

	void FirstLevel::startSzeptuchaEncounter(Core::Engine* engine, const Vector2& corpse_position) {
		if (!engine || !engine->getPlayer())
			return;

		if (engine->getUIHandler().isDialogueOpen()) {
			queueSzeptuchaEncounter(corpse_position);
			return;
		}

		removeIntroNpc();

		const auto player = engine->getPlayer();
		const auto& szeptucha_config = F::introConfig()["szeptucha"];
		const Vector2 player_center = player->getCenter();
		Vector2 spawn_direction = {
			corpse_position.x - player_center.x,
			corpse_position.y - player_center.y
		};
		const float direction_length = std::sqrt(spawn_direction.x * spawn_direction.x + spawn_direction.y * spawn_direction.y);
		if (direction_length > 0.001f) {
			spawn_direction.x /= direction_length;
			spawn_direction.y /= direction_length;
		} else {
			spawn_direction = {1.0f, 0.0f};
		}

		const auto& spawn_offset = szeptucha_config["spawn_offset"];
		const float forward_offset = spawn_offset.value("x", F::SZEPTUCHA_DEFAULT_FORWARD_OFFSET);
		const float side_offset = spawn_offset.value("y", F::SZEPTUCHA_DEFAULT_SIDE_OFFSET);
		const Vector2 side_direction = {-spawn_direction.y, spawn_direction.x};
		Vector3 spawn_position = {
			corpse_position.x + spawn_direction.x * forward_offset + side_direction.x * side_offset,
			player->getAltitude(),
			corpse_position.y + spawn_direction.y * forward_offset + side_direction.y * side_offset
		};
		if (engine->getCurrentMap() && engine->getCurrentMap()->getNavMesh().isReady())
			spawn_position = engine->getCurrentMap()->getNavMesh().getClosestWalkablePosition(spawn_position);

		auto szeptucha = std::make_shared<Entity::SzeptuchaNpc>("Szeptucha", spawn_position.x, spawn_position.z, engine);
		szeptucha->setAltitude(spawn_position.y);
		szeptucha->setAudioManager(&engine->getAudioManager());
		szeptucha->setDormant(false);
		szeptucha->rotateTowardsCenter(engine->getPlayer()->getCenter().x, engine->getPlayer()->getCenter().y);
		engine->getPlayer()->rotateTowardsCenter(szeptucha->getCenter().x, szeptucha->getCenter().y);
		engine->spawnEntity(szeptucha);
		Core::Logger::debugLog(
			"FirstLevel: spawned Szeptucha at (" +
			std::to_string(spawn_position.x) + ", " +
			std::to_string(spawn_position.y) + ", " +
			std::to_string(spawn_position.z) + "), corpse=(" +
			std::to_string(corpse_position.x) + ", " +
			std::to_string(corpse_position.y) + ")");
		_intro_npc = szeptucha;
		_intro_flash_timer = 0.55f;
		_intro_phase = IntroPhase::SzeptuchaDialogue;
		_intro_timer = 0.0f;

		Game::DialogueTree tree = F::buildLinearDialogueTreeFromJson(
			szeptucha_config["lines"],
			szeptucha_config.value("final_option", "Co?"));

		engine->getUIHandler().openDialogue(tree, 0, [this](const int, const bool) {
			removeIntroNpc();
			_intro_flash_timer = 0.55f;
			_pending_final_dialogue = true;
		});
	}

	void FirstLevel::openFinalDialogue(Core::Engine* engine) {
		if (!engine)
			return;

		Game::DialogueTree tree = F::buildSingleNodeDialogueTreeFromJson(F::introConfig()["final_dialogue"]);

		_intro_dialogue_opened = true;
		_intro_phase = IntroPhase::FinalDialogue;
		engine->getUIHandler().openDialogue(tree, 0, [this, engine](const int, const bool) {
			finishIntroSequence(engine);
		});
	}

	void FirstLevel::finishIntroSequence(Core::Engine* engine) {
		stopSlideVoice(engine);
		_intro_phase = IntroPhase::Inactive;
		_intro_timer = 0.0f;
		_intro_overlay_alpha = 0.0f;
		_intro_dialogue_opened = false;
		_intro_flash_timer = 0.0f;
		_pending_szeptucha_encounter = false;
		_pending_final_dialogue = false;
		_pending_corpse_completion = false;
		_pending_szeptucha_delay = 0.0f;
	}

	void FirstLevel::startOutroSequence(Core::Engine* engine) {
		if (!engine)
			return;

		stopSlideVoice(engine);
		removeIntroNpc();
		_intro_slides.clear();

		const auto& config = F::outroConfig();
		if (config.contains("slides") && config["slides"].is_array()) {
			for (const auto& slide_json : config["slides"]) {
				IntroSlide slide;
				slide.text = slide_json.value("text", "");
				slide.voice_path = slide_json.value("voice_path", "");
				slide.image_path = slide_json.value("image_path", "");
				slide.duration = F::resolveSlideDuration(slide_json, 7.0f);
				_intro_slides.push_back(std::move(slide));
			}
		}

		for (auto& slide : _intro_slides) {
			if (!slide.image_path.empty()) {
				slide.image_texture = engine->getResourceManager().getTexture(slide.image_path);
				if (slide.image_texture && slide.image_texture->id > 0)
					SetTextureFilter(*slide.image_texture, TEXTURE_FILTER_TRILINEAR);
			}
		}

		_intro_slide_index = 0;
		_intro_phase = _intro_slides.empty() ? IntroPhase::OutroFadeToMenu : IntroPhase::OutroFadeFromGame;
		_intro_timer = 0.0f;
		_intro_overlay_alpha = 0.0f;
		_intro_dialogue_opened = false;
		_intro_flash_timer = 0.0f;
		_pending_szeptucha_encounter = false;
		_pending_final_dialogue = false;
		_pending_corpse_completion = false;
		_pending_szeptucha_delay = 0.0f;

		if (const auto player = engine->getPlayer())
			player->stop();
	}

	void FirstLevel::finishOutroSequence(Core::Engine* engine) {
		stopSlideVoice(engine);
		_intro_timer = 0.0f;
		_intro_overlay_alpha = 1.0f;
		_intro_slides.clear();
		_intro_slide_index = 0;

		if (engine)
			engine->requestReturnToMainMenu(F::outroConfig().value("beta_message", "Twierdza Kamienna nie jest dostepna w wersji Beta."));
	}

	void FirstLevel::removeIntroNpc() {
		if (const auto npc = _intro_npc.lock())
			npc->setDormant(true);
		_intro_npc.reset();
	}

	void FirstLevel::playSlideVoice(Core::Engine* engine) {
		stopSlideVoice(engine);
		if (!engine || _intro_slide_index >= _intro_slides.size())
			return;

		const auto& slide = _intro_slides[_intro_slide_index];
		if (slide.voice_path.empty())
			return;

		_intro_slide_voice_id = "intro_slide:" + slide.voice_path;
		engine->getAudioManager().playSoundFile(_intro_slide_voice_id, slide.voice_path, {F::SLIDE_VOICE_VOLUME, 1.0f, true});
	}

	void FirstLevel::stopSlideVoice(Core::Engine* engine) {
		if (engine && !_intro_slide_voice_id.empty())
			engine->getAudioManager().stopSound(_intro_slide_voice_id);
		_intro_slide_voice_id.clear();
	}

} // namespace Nawia::World
