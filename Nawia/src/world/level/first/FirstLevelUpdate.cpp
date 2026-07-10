#include "FirstLevel.h"

#include "FirstLevelInternal.h"

#include <Engine.h>
#include <Player.h>
#include <QuestManager.h>
#include <UIHandler.h>

#include <algorithm>

namespace Nawia::World {

	namespace F = FirstLevelSupport;

	void FirstLevel::update(Core::Engine* engine, const float dt) {
		Level::update(engine, dt);

		if (_intro_phase == IntroPhase::Inactive || !engine)
			return;

		if ((_intro_phase == IntroPhase::Slides || _intro_phase == IntroPhase::FadeFromBlackAfterSlides) &&
			IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			const Rectangle skip_rect = F::getIntroSkipButtonRect(GetScreenWidth(), GetScreenHeight());
			if (CheckCollisionPointRec(GetMousePosition(), skip_rect)) {
				skipIntroSlides(engine);
				return;
			}
		}

		if (_intro_phase == IntroPhase::OutroSlides && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			const Rectangle skip_rect = F::getIntroSkipButtonRect(GetScreenWidth(), GetScreenHeight());
			if (CheckCollisionPointRec(GetMousePosition(), skip_rect)) {
				skipOutroSlides(engine);
				return;
			}
		}

		if (_pending_szeptucha_delay > 0.0f)
			_pending_szeptucha_delay = std::max(0.0f, _pending_szeptucha_delay - dt);

		if (!engine->getUIHandler().isDialogueOpen()) {
			if (_pending_szeptucha_encounter) {
				if (_pending_szeptucha_delay > 0.0f)
					return;

				const Vector2 position = _pending_szeptucha_position;
				_pending_szeptucha_encounter = false;
				if (_pending_corpse_completion) {
					_pending_corpse_completion = false;
					engine->getQuestManager().completeQuest("inspect_wanda_body", engine);
				}
				startSzeptuchaEncounter(engine, position);
				return;
			}

			if (_pending_final_dialogue) {
				_pending_final_dialogue = false;
				openFinalDialogue(engine);
				return;
			}
		}

		if (auto player = engine->getPlayer(); player && _intro_phase != IntroPhase::InspectCorpse)
			player->stop();

		if (_intro_flash_timer > 0.0f)
			_intro_flash_timer = std::max(0.0f, _intro_flash_timer - dt);

		_intro_timer += dt;
		switch (_intro_phase) {
			case IntroPhase::Slides:
			case IntroPhase::OutroSlides:
				_intro_overlay_alpha = 1.0f;
				if (_intro_slide_index < _intro_slides.size() &&
					_intro_timer >= _intro_slides[_intro_slide_index].duration) {
					_intro_slide_index++;
					_intro_timer = 0.0f;
					if (_intro_slide_index < _intro_slides.size()) {
						playSlideVoice(engine);
					} else if (_intro_phase == IntroPhase::OutroSlides) {
						stopSlideVoice(engine);
						_intro_phase = IntroPhase::OutroFadeToMenu;
						_intro_timer = 0.0f;
						_intro_overlay_alpha = 0.0f;
					} else {
						stopSlideVoice(engine);
						if (const auto player = engine->getPlayer()) {
							player->stop();
							const int death_frames = player->getAnimationFrameCount("Death01");
							player->setAnimationSpeed(0.0f);
							player->playAnimation("Death01", false, true, std::max(0, death_frames - 1), true);
						}
						_intro_phase = IntroPhase::FadeFromBlackAfterSlides;
						_intro_overlay_alpha = 1.0f;
					}
				}
				break;
			case IntroPhase::FadeFromBlackAfterSlides:
				_intro_overlay_alpha = std::max(0.38f, 1.0f - _intro_timer / 2.4f);
				if (_intro_timer >= 2.4f)
					openAwakeningDialogue(engine);
				break;
			case IntroPhase::AwakeningDialogue:
				_intro_overlay_alpha = 0.38f;
				break;
			case IntroPhase::InspectCorpse:
				_intro_overlay_alpha = 0.0f;
				break;
			case IntroPhase::SzeptuchaDialogue:
			case IntroPhase::FinalDialogue:
				_intro_overlay_alpha = 0.22f;
				break;
			case IntroPhase::OutroFadeFromGame:
				_intro_overlay_alpha = std::clamp(_intro_timer / 1.35f, 0.0f, 1.0f);
				if (_intro_timer >= 1.35f) {
					_intro_phase = IntroPhase::OutroSlides;
					_intro_timer = 0.0f;
					_intro_overlay_alpha = 1.0f;
					_intro_slide_index = 0;
					playSlideVoice(engine);
				}
				break;
			case IntroPhase::OutroFadeToMenu:
				_intro_overlay_alpha = std::clamp(_intro_timer / 2.5f, 0.0f, 1.0f);
				if (_intro_timer >= 2.5f)
					finishOutroSequence(engine);
				break;
			case IntroPhase::Inactive:
				break;
		}
	}

} // namespace Nawia::World
