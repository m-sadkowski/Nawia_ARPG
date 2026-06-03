#pragma once

#include <StoryNpc.h>

#include <vector>

namespace Nawia::Entity {

	/**
	 * @brief Questowy NPC, ktory rozmawia, idzie po waypointach, wraca do domu i budzi uratowane grzyby.
	 *
	 * Kontrakt animacji jest celowo waski: idle poza akcja, walk podczas ruchu,
	 * talk w trakcie interakcji. Stan trasy zostaje tutaj, bo to zachowanie
	 * questowe Mushrooma, a nie ogolna odpowiedzialnosc StoryNpc.
	 */
	class MushroomNpc : public StoryNpc {
	public:
		MushroomNpc(
			const std::string& name,
			float x,
			float y,
			Core::Engine* engine,
			const std::string& follow_checkpoint_name = "Checkpoint Gziba");
		[[nodiscard]] const char* getNpcClass() const override { return "mushroom"; }
		[[nodiscard]] bool shouldNotifyQuestTalkOnDialogueComplete() const override;
		void onInteract(Entity& instigator) override;
		[[nodiscard]] bool canInteract() const override;
		void update(float delta_time) override;
		void handleQuestTalkCompleted(Core::Engine& engine) override;

	private:
		/** @brief Wysokopoziomowa trasa obslugiwana przez logike questa Mushrooma. */
		enum class TravelMode {
			None,
			ToBrothers,
			ReturningHome
		};

		/** @brief Dobiera stage dialogu z aktualnego stanu questa/postepu. */
		void refreshDialogue();
		/** @brief Obsluguje trase po tym, jak Mushroom zostanie towarzyszem. */
		void updateCompanionTravel(float delta_time);
		void startRoute(TravelMode mode);
		void advanceRoute();
		void buildPathToPoint(Vector2 target);
		void trimCurrentPathStart();
		void updatePathMovement(float delta_time);
		void stopPathMovement();
		void sendPurifiedFollowersHome();
		void rotateToPlayerOnInterval(float delta_time);
		void playIdleAnimation();
		void playWalkAnimation();
		void playTalkAnimation();
		[[nodiscard]] std::vector<Vector2> collectOrderedFollowWaypoints(bool reverse_to_home) const;
		[[nodiscard]] int getRescuedMushroomCount() const;
		[[nodiscard]] int getRequiredRescueCount() const;
		[[nodiscard]] bool areAllMushroomsAlreadyRescued() const;
		[[nodiscard]] bool hasDialogueAvailable() const;

		std::string _follow_checkpoint_name; ///< Nazwany checkpoint uzywany jako cel trasy.
		bool _reached_follow_checkpoint = false;
		bool _follow_path_requested = false; ///< Chroni przed budowaniem tej samej trasy co klatke.
		bool _return_started = false;
		float _look_at_player_timer = 0.0f; ///< Ogranicza czestosc obracania sie do pobliskiego gracza.
		std::vector<Vector2> _current_path; ///< Sciezka navmesha dla aktualnego odcinka.
		std::vector<Vector2> _travel_waypoints;
		size_t _travel_waypoint_index = 0;
		TravelMode _travel_mode = TravelMode::None;
		Vector2 _home_position = {0.0f, 0.0f};
	};

} // namespace Nawia::Entity
