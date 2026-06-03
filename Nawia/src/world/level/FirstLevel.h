#pragma once

#include <Level.h>

#include <memory>
#include <string>
#include <vector>

namespace Nawia::Entity { class Entity; }

namespace Nawia::World {

	/**
	 * @brief Fabularny poziom Wczora z wlasna sekwencja intro.
	 *
	 * Wiekszosc lokacji korzysta z ogolnego zachowania Level. FirstLevel trzyma
	 * dodatkowy stan slajdow, inspekcji zwlok, spawnu/dialogu Szeptuchy oraz
	 * tymczasowych ustawien kamery podczas przebudzenia.
	 */
	class FirstLevel : public Level {
	public:
		void onEnter(Core::Engine* engine) override;
		void onExit(Core::Engine* engine) override;
		void onNewGameStarted(Core::Engine* engine) override;
		void update(Core::Engine* engine, float dt) override;
		void handleStoryEvent(Core::Engine* engine, const std::string& event_id, Vector2 world_position) override;
		void renderOverlay(Core::Engine* engine) const override;

		[[nodiscard]] std::string getName() const override { return "Wczora"; }
		[[nodiscard]] std::vector<std::string> getLocations() const override { return {"Wczora"}; }
		[[nodiscard]] std::vector<LevelLocationFile> getLocationFiles() const override;
		[[nodiscard]] std::string getDefaultInitialLocation() const override { return "Wczora"; }
		[[nodiscard]] bool blocksPlayerControl() const override;
		[[nodiscard]] bool isInteractionOnly() const override;
		[[nodiscard]] float getCameraZoomMultiplier() const override;
		[[nodiscard]] float getCameraTargetHeightMultiplier() const override;

	private:
		/** @brief Liniowa maszyna stanow otwierajacej sekwencji Wczora. */
		enum class IntroPhase {
			Inactive,
			Slides,
			FadeFromBlackAfterSlides,
			AwakeningDialogue,
			InspectCorpse,
			SzeptuchaDialogue,
			FinalDialogue
		};

		/** @brief Pojedynczy slajd intro z opcjonalnym glosem i zaladowanym obrazem. */
		struct IntroSlide {
			std::string text;
			std::string voice_path;
			std::string image_path;
			std::shared_ptr<Texture2D> image_texture;
			float duration = 6.0f;
		};

		void startIntroSequence(Core::Engine* engine);
		void spawnIntroCorpse(Core::Engine* engine);
		void queueCorpseInspected(const Vector2& corpse_position);
		void queueSzeptuchaEncounter(const Vector2& corpse_position);
		void startSzeptuchaEncounter(Core::Engine* engine, const Vector2& corpse_position);
		void openAwakeningDialogue(Core::Engine* engine);
		void openFinalDialogue(Core::Engine* engine);
		void finishIntroSequence(Core::Engine* engine);
		void removeIntroNpc();
		void playSlideVoice(Core::Engine* engine);
		void stopSlideVoice(Core::Engine* engine);

		IntroPhase _intro_phase = IntroPhase::Inactive;
		float _intro_timer = 0.0f;         ///< Wspolny timer aktywnej fazy intro.
		float _intro_overlay_alpha = 0.0f; ///< Alfa czarnego overlayu uzywana przez fade'y.
		bool _intro_dialogue_opened = false;
		std::weak_ptr<Entity::Entity> _intro_npc;    ///< Tymczasowy NPC intro, usuwany po zakonczeniu.
		std::weak_ptr<Entity::Entity> _intro_corpse; ///< Tymczasowe zwloki ogladane przez gracza.
		std::vector<IntroSlide> _intro_slides;
		size_t _intro_slide_index = 0;
		std::string _intro_slide_voice_id; ///< Aktualnie odtwarzany glos slajdu w AudioManagerze.
		float _intro_flash_timer = 0.0f;
		bool _pending_szeptucha_encounter = false;
		bool _pending_final_dialogue = false;
		bool _pending_corpse_completion = false;
		float _pending_szeptucha_delay = 0.0f;
		Vector2 _pending_szeptucha_position = {0.0f, 0.0f};
	};

} // namespace Nawia::World
