#pragma once

#include <AbilityStats.h>
#include <Equipment.h>

#include <json.hpp>
#include <raylib.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Nawia::Entity {
	class Ability;
	class Collider;
}

namespace Nawia::Item {
	class ItemDatabase;
}

namespace Nawia::Audio {
	class AudioManager;
}

namespace Nawia::Core {
	class ResourceManager;
}

namespace Nawia::Entity {

	/**
	 * @enum EntityType
	 * @brief Określa główną kategorię encji w świecie gry.
	 */
	enum class EntityType {
		None,
		Player,
		Enemy,
		Ally,
		NPCActor,
		NPCStatic,
		Projectile, ///< Efekt umiejętności działający jak encja.
		Trigger,    ///< Obszar aktywujący logikę, np. checkpoint.
		Chest,      ///< Interaktywny pojemnik z ekwipunkiem.
		Item
	};

	/**
	 * @enum Faction
	 * @brief Określa relacje sojusz/wrogość używane w walce i AI.
	 */
	enum class Faction {
		Player,  ///< Encje kontrolowane przez gracza.
		Enemy,   ///< Encje wrogie wobec gracza i sojuszników.
		Neutral, ///< Encje neutralne, zwykle poza walką.
		Ally,    ///< Encje walczące po stronie gracza.
		None     ///< Brak frakcji, np. obiekt bez udziału w walce.
	};

	struct AnimationBundle {
		std::vector<ModelAnimation> clips;
		~AnimationBundle();
	};

	struct AnimationClipRef {
		std::shared_ptr<const AnimationBundle> bundle;
		int clip_index = 0;
	};

	/**
	 * @class Entity
	 * @brief Bazowa klasa wszystkich obiektów świata gry.
	 *
	 * `Entity` zbiera wspólne mechanizmy: pozycję, ruch, HP, animacje, model 3D,
	 * kolider, frakcję, wybieranie celu oraz listę umiejętności. Pozycja `Vector2`
	 * mapuje się na płaszczyznę świata 3D jako `{x, wysokość, y}`.
	 */
	class Entity : public std::enable_shared_from_this<Entity> {
	public:
		/**
		 * @brief Tworzy encję z podstawowymi danymi używanymi przez świat gry.
		 * @param name Nazwa encji wyświetlana i używana w logach.
		 * @param start_x Początkowa pozycja X w świecie.
		 * @param start_y Początkowa pozycja Y, mapowana na oś Z świata 3D.
		 * @param texture Tekstura pomocnicza lub awaryjna tekstura 2D.
		 * @param max_hp Maksymalne i początkowe HP.
		 */
		Entity(
			const std::string& name,
			float start_x,
			float start_y,
			const std::shared_ptr<Texture2D>& texture,
			int max_hp);

		virtual ~Entity();

		/**
		 * @brief Aktualizuje bazowy ruch, animacje i stan śmierci encji.
		 * @param delta_time Czas od poprzedniej klatki w sekundach.
		 */
		virtual void update(float delta_time);

		/**
		 * @brief Renderuje model encji i diagnostyczne kolidery w trybie 3D.
		 * @param camera Kamera używana do diagnostycznego renderowania koliderów.
		 */
		virtual void render(const Camera3D& camera);

		// Pozycja i współrzędne.
		[[nodiscard]] float getX() const { return _pos.x; }
		[[nodiscard]] float getY() const { return _pos.y; }
		[[nodiscard]] float getAltitude() const { return _altitude; }
		void setX(float x) { _pos.x = x; }
		void setY(float y) { _pos.y = y; }
		void setAltitude(float altitude) { _altitude = altitude; }
		[[nodiscard]] Vector2 getCenter() const;
		virtual void applyRoot(float duration);
		void applyPoison(float duration, int damage_per_tick, float tick_interval = 1.0f);
		void clearStatusEffects();
		[[nodiscard]] bool isMovementRooted() const { return _root_timer > 0.0f; }
		[[nodiscard]] bool isPoisoned() const { return _poison_timer > 0.0f; }
		[[nodiscard]] float getRootRemaining() const { return _root_timer; }
		[[nodiscard]] float getPoisonRemaining() const { return _poison_timer; }

		/**
		 * @brief Zwraca pozycję encji w świecie 3D.
		 */
		[[nodiscard]] virtual Vector3 getWorldPos3D() const { return {_pos.x, _altitude, _pos.y}; }

		/**
		 * @brief Projektuje pozycję świata na współrzędne ekranu.
		 */
		[[nodiscard]] Vector2 getScreenPosition(const Camera3D& camera) const;

		/**
		 * @brief Sprawdza, czy promień myszy trafia w obszar kliknięcia encji.
		 */
		[[nodiscard]] virtual bool isMouseOver(float screen_x, float screen_y, const Camera3D& camera) const;

		/**
		 * @brief Zwraca pudełko ograniczające modelu w przestrzeni świata albo kształt awaryjny.
		 */
		[[nodiscard]] BoundingBox getBoundingBox() const;

		/**
		 * @brief Buduje macierz transformacji używaną przy renderingu i testach promieniem.
		 */
		[[nodiscard]] Matrix getWorldTransformMatrix() const;

		/**
		 * @brief Sprawdza, czy promień trafia w którąkolwiek siatkę modelu.
		 */
		[[nodiscard]] bool checkRayHitsMesh(const Ray& ray) const;

		/**
		 * @brief Zwraca najbliższe trafienie promienia w siatkę modelu.
		 */
		[[nodiscard]] RayCollision getRayMeshCollision(const Ray& ray) const;
		[[nodiscard]] bool isVisibleInCamera(const Camera3D& camera, float screen_margin = 96.0f) const;

		// Transformacja i ruch.
		void setVelocity(float x, float y) { _velocity.x = x; _velocity.y = y; }
		[[nodiscard]] Vector2 getVelocity() const { return _velocity; }
		void setScale(float scale) { _scale = scale; }
		[[nodiscard]] float getScale() const { return _scale; }
		void setHovered(bool hovered) { _hovered = hovered; }
		void setAudioManager(Audio::AudioManager* audio_manager) { _audio_manager = audio_manager; }
		void hideMeshIndex(int mesh_index);

		/**
		 * @brief Ustawia docelowy punkt ruchu encji.
		 */
		virtual void moveTo(float x, float y);

		/**
		 * @brief Przesuwa encję w stronę punktu ustawionego przez `moveTo`.
		 */
		virtual void updateMovement(float dt);

		void setMovementSpeed(float speed) { _movement_speed = speed; }
		[[nodiscard]] float getMovementSpeed() const { return _movement_speed; }

		void setSpeedMultiplier(float multiplier) { _speed_multiplier = multiplier; }
		[[nodiscard]] float getSpeedMultiplier() const { return _speed_multiplier; }
		void setDamageMultiplier(float multiplier) { _damage_multiplier = multiplier; }
		[[nodiscard]] float getDamageMultiplier() const { return _damage_multiplier; }

		// HP i obrażenia.
		/**
		 * @brief Zadaje obrażenia i uruchamia sekwencję śmierci, jeśli HP spadnie do zera.
		 * @param dmg Liczba punktów obrażeń.
		 */
		virtual void takeDamage(int dmg);

		/**
		 * @brief Natychmiast oznacza encję jako martwą.
		 */
		void die();

		[[nodiscard]] bool isDead() const { return _hp <= 0; }
		[[nodiscard]] bool isDying() const { return _is_dying; }
		[[nodiscard]] bool shouldPersistAfterDeath() const { return _persist_after_death; }
		void setPersistAfterDeath(bool value) { _persist_after_death = value; }
		[[nodiscard]] bool isMoving() const { return _is_moving; }
		[[nodiscard]] int getHP() const { return _hp; }
		[[nodiscard]] int getMaxHP() const { return _max_hp; }
		void setHP(int hp);
		[[nodiscard]] const std::string& getName() const { return _name; }
		void setName(const std::string& name) { _name = name; }

		// Model 3D i animacje.
		/**
		 * @brief Ładuje model 3D i rejestruje animację domyślną z tego pliku.
		 */
		void loadModel(const std::string& path, bool rotate_model = false);

		/**
		 * @brief Podmienia sam model, zostawiajac aktualnie zaladowane animacje.
		 */
		void replaceModel(const std::string& path, bool rotate_model = false);

		/**
		 * @brief Podpina wspolny, nieposiadany model z cache dla nieanimowanych encji.
		 */
		void useSharedModel(const Model& model);

		/**
		 * @brief Skaluje zaladowany model do docelowej wysokosci, centrujac go na osi X/Z i opierajac o ziemie.
		 */
		bool fitLoadedModelToHeight(float target_height, bool center_xz = true, bool align_to_ground = true);

		/**
		 * @brief Rejestruje animację pod nazwą używaną później w `playAnimation`.
		 */
		void addAnimation(const std::string& name, const std::string& path);
		void addAnimation(const std::string& name, const std::string& path, int clip_index);

		void loadAnimationBundle(const std::string& path);

		/**
		 * @brief Wczytuje dane animacji do wspolnego cache bez tworzenia encji.
		 */
		static void preloadAnimationData(const std::string& path);

		/**
		 * @brief Ustawia manager zasobow uzywany przez loadModel do wspoldzielenia meshy.
		 */
		static void setSharedResourceManager(Core::ResourceManager* manager);

		[[nodiscard]] static Core::ResourceManager* getSharedResourceManager();
		static void setAudioListener(const std::shared_ptr<Entity>& listener);

		/**
		 * @brief Odtwarza zarejestrowaną animację.
		 * @param name Nazwa animacji dodana przez `addAnimation`.
		 * @param loop Czy animacja ma zapętlać się po dojściu do końca.
		 * @param lock_movement Czy animacja blokuje ruch do czasu zakończenia.
		 * @param start_frame Klatka startowa animacji.
		 * @param force Czy wymusić restart nawet, gdy ta animacja już gra.
		 */
		void playAnimation(
			const std::string& name,
			bool loop = true,
			bool lock_movement = false,
			int start_frame = 0,
			bool force = false);
		void playAnimationPingPong(
			const std::string& name,
			bool lock_movement = true,
			int start_frame = 0,
			bool force = false);
		void playAnimationFreezeOnLastFrame(
			const std::string& name,
			bool lock_movement = false,
			int start_frame = 0,
			bool force = true);

		void setAnimationSpeed(float multiplier) { _anim_speed_multiplier = multiplier; }
		[[nodiscard]] float getAnimationSpeed() const { return _anim_speed_multiplier; }
		[[nodiscard]] int getAnimationFrameCount(const std::string& name) const;
		void applyCurrentAnimationFrame();
		[[nodiscard]] bool isAnimationLocked() const { return _anim_locked; }
		static constexpr float ANIMATION_DURATION_SCALE = 1.5f;

		void setRotation(float angle) { _rotation = angle; }
		[[nodiscard]] float getRotation() const { return _rotation; }

		/**
		 * @brief Ustawia wizualny offset modelu względem matematycznego kierunku patrzenia.
		 */
		void setModelFacingOffset(float deg) { _model_facing_offset = deg; }
		[[nodiscard]] float getModelFacingOffset() const { return _model_facing_offset; }

		/**
		 * @brief Obraca encję w stronę punktu świata.
		 */
		void rotateTowards(float world_x, float world_y);

		// Kolidery.
		/**
		 * @brief Podpina kolider używany do kolizji, triggerów lub renderowania diagnostycznego.
		 */
		void setCollider(std::unique_ptr<Collider> collider);

		/**
		 * @brief Obraca encję w stronę punktu świata, licząc kierunek od środka encji.
		 */
		void rotateTowardsCenter(float world_x, float world_y);

		[[nodiscard]] Collider* getCollider() const { return _collider.get(); }

		/**
		 * @brief Globalny przełącznik renderowania koliderów i pudełek ograniczających.
		 */
		static bool DebugColliders;

		// Umiejętności.
		/**
		 * @brief Wczytuje statystyki umiejętności z `abilities.json`.
		 */
		static AbilityStats getAbilityStatsFromJson(const std::string& name);

		/**
		 * @brief Dodaje umiejętność i ustawia tę encję jako źródło użycia.
		 */
		void addAbility(const std::shared_ptr<Ability>& ability);
		void setAbility(int index, const std::shared_ptr<Ability>& ability);

		/**
		 * @brief Zwraca umiejętność z danego slotu albo `nullptr`.
		 */
		[[nodiscard]] std::shared_ptr<Ability> getAbility(int index);

		[[nodiscard]] const std::vector<std::shared_ptr<Ability>>& getAbilities() const { return _abilities; }

		/**
		 * @brief Aktualizuje czasy odnowienia i stan wszystkich umiejętności.
		 */
		void updateAbilities(float dt) const;

		// Encje tworzone w trakcie aktualizacji.
		/**
		 * @brief Dodaje encję, którą silnik ma dopiąć do świata po zakończeniu aktualizacji.
		 */
		void addPendingSpawn(std::shared_ptr<Entity> entity) {
			if (entity)
				_pending_spawns.push_back(std::move(entity));
		}

		/**
		 * @brief Zwraca listę encji oczekujących na dodanie do świata.
		 */
		[[nodiscard]] const std::vector<std::shared_ptr<Entity>>& getPendingSpawns() const { return _pending_spawns; }

		/**
		 * @brief Czyści listę encji oczekujących po ich odebraniu przez silnik.
		 */
		void clearPendingSpawns() { _pending_spawns.clear(); }

		// Frakcje i wybieranie celu.
		[[nodiscard]] Faction getFaction() const { return _faction; }
		void setFaction(Faction faction) { _faction = faction; }

		/**
		 * @brief Ustawia aktualny cel encji.
		 */
		virtual void setTarget(const std::shared_ptr<Entity>& target) { _target = target; }

		/**
		 * @brief Zwraca aktualny cel encji albo `nullptr`.
		 */
		[[nodiscard]] std::shared_ptr<Entity> getTarget() const { return _target.lock(); }

		/**
		 * @brief Zapamietuje encje, ktora ostatnio zadala obrazenia.
		 *
		 * Trzymamy weak_ptr, zeby AI moglo preferowac ostatniego agresora bez
		 * wydluzania jego cyklu zycia.
		 */
		void rememberDamageSource(Entity* source) {
			_last_damage_source = source ? source->weak_from_this() : std::weak_ptr<Entity>{};
		}

		[[nodiscard]] std::shared_ptr<Entity> getLastDamageSource() const { return _last_damage_source.lock(); }

		/**
		 * @brief Zwraca odległość do celu albo bardzo dużą wartość, gdy celu nie ma.
		 */
		[[nodiscard]] float getDistanceToTarget() const;

		/**
		 * @brief Zwraca pozycję celu albo pozycję własną, gdy cel nie istnieje.
		 */
		[[nodiscard]] Vector2 getTargetPosition() const;

		/**
		 * @brief Sprawdza, czy cel istnieje i nie jest martwy.
		 */
		[[nodiscard]] bool hasValidTarget() const;

		static constexpr float DEFAULT_PATH_RECALC_INTERVAL = 0.5f;

		/**
		 * @brief Przelicza cel ruchu co określony czas i podąża za wskazaną encją.
		 */
		void chaseTarget(float dt, float path_recalc_interval = DEFAULT_PATH_RECALC_INTERVAL);

		[[nodiscard]] EntityType getType() const { return _type; }
		void setType(EntityType type) { _type = type; }
		void setMaxHp(int max_hp);

		// Uśpienie.
		/**
		 * @brief Ustawia stan uśpienia encji.
		 *
		 * Uśpiona encja nie renderuje się, nie aktualizuje AI i jest pomijana przez
		 * mechaniki aktywnych lokacji. Używa tego `SpawnManager` przy przełączaniu
		 * lokacji oraz tworzeniu encji po zbliżeniu.
		 */
		void setDormant(bool dormant) { _dormant = dormant; }
		[[nodiscard]] bool isDormant() const { return _dormant; }
		[[nodiscard]] virtual bool shouldWakeOnLocationChange() const { return true; }

		// Zapis stanu encji.
		/**
		 * @brief Zapisuje bazowy runtime'owy stan encji do JSON-a.
		 *
		 * Klasy pochodne moga rozszerzac wynik wlasnymi polami, np. otwarciem
		 * skrzyni albo postepem questa NPC.
		 */
		[[nodiscard]] virtual nlohmann::json serializeState() const;

		/**
		 * @brief Przywraca bazowy runtime'owy stan encji z JSON-a.
		 *
		 * Klasy pochodne moga uzyc `item_database` do odtworzenia ekwipunku,
		 * dlatego parametr jest opcjonalny.
		 */
		virtual void applyState(const nlohmann::json& state, Item::ItemDatabase* item_database = nullptr);

	protected:
		template <typename T> friend class EntityBuilder;
		Entity();

		Vector2 _pos = {0.0f, 0.0f};
		float _altitude = 0.0f;
		Vector2 _velocity = {0.0f, 0.0f};
		float _scale = 1.0f;
		std::shared_ptr<Texture2D> _texture;
		EntityType _type = EntityType::None;
		Audio::AudioManager* _audio_manager = nullptr;

		std::unique_ptr<Collider> _collider;
		std::vector<std::shared_ptr<Entity>> _pending_spawns;

		int _hp = 1;
		int _max_hp = 1;

	public:
		/**
		 * @brief Zwraca model 3D do niskopoziomowych operacji raylib.
		 */
		[[nodiscard]] Model& getModel() { return _model; }

		/**
		 * @brief Sprawdza, czy encja ma poprawnie załadowany model 3D.
		 */
		[[nodiscard]] bool hasModelLoaded() const { return _model_loaded; }

		void playSoundEffect(const std::string& id, float volume = 1.0f, bool restart_if_playing = true, float pitch = 1.0f) const;
		void stopSoundEffect(const std::string& id) const;

	protected:
		// Dane modelu i animacji.
		Model _model = {};
		std::vector<AnimationClipRef> _animations;
		std::map<std::string, int> _animation_map;
		std::map<std::string, int> _animation_path_map;

		int _current_anim_index = 0;
		float _anim_frame_counter = 0.0f;
		int _last_applied_anim_index = -1;
		int _last_applied_anim_frame = -1;
		float _anim_speed_multiplier = 1.0f;
		float _anim_fps = 60.0f;
		float _rotation = 0.0f;
		float _model_facing_offset = 90.0f; ///< Offset modelu względem kierunku matematycznego.
		bool _model_loaded = false;
		bool _owns_model = false;
		bool _cloned_model = false; ///< Model pochodzi z cloneModel — nie zwalniaj tekstur.
		BoundingBox _local_model_bounding_box = {}; ///< Granice w przestrzeni modelu po load/replace/fit.
		bool _local_model_bounding_box_valid = false;
		bool _anim_looping = true;
		bool _anim_locked = false;
		bool _anim_ping_pong = false;
		bool _anim_reverse_phase = false;
		bool _freeze_animation_on_completion = false;
		bool _animation_frozen_at_last_frame = false;
		float _anim_direction = 1.0f;
		bool _persist_after_death = false;
		bool _hovered = false;
		bool _dormant = false;
		std::vector<int> _hidden_mesh_indices;

		bool _is_dying = false;
		std::string _death_anim_name = "death";

		// Stan ruchu.
		bool _is_moving = false;
		float _movement_speed = 2.0f;
		float _target_x = 0.0f;
		float _target_y = 0.0f;

		float _speed_multiplier = 1.0f;
		float _damage_multiplier = 1.0f;
		float _root_timer = 0.0f;
		float _poison_timer = 0.0f;
		float _poison_tick_timer = 0.0f;
		float _poison_tick_interval = 1.0f;
		int _poison_damage_per_tick = 0;

		// Śledzenie celu.
		std::weak_ptr<Entity> _target;             ///< Aktualny cel AI/walki, nieposiadany.
		std::weak_ptr<Entity> _last_damage_source; ///< Ostatni agresor uzywany przy wyborze celu.
		float _path_recalc_timer = 0.0f;

		Faction _faction = Faction::None;
		std::string _name;

		void updateAnimation(float dt);
		void updateStatusEffects(float dt);
		void updateMovementSound(const std::string& path, bool should_play, float volume = 0.55f, float pitch = 1.0f);
		[[nodiscard]] float getSpatialAudioVolumeMultiplier() const;
		void unloadModelData();
		virtual void onDeathStarted() {}

		std::string _movement_sound_id;

		std::vector<std::shared_ptr<Ability>> _abilities;

		std::unique_ptr<Item::Equipment> _equipment;
	};

	/**
	 * @class EntityBuilder
	 * @brief Bazowy builder ustawiający wspólne pola encji przed oddaniem obiektu.
	 */
	template <typename Derived>
	class EntityBuilder {
	public:
		EntityBuilder() = default;

		/** @brief Ustawia nazwę encji. */
		Derived& setName(const std::string& name) {
			_entity->_name = name;
			return self();
		}

		/** @brief Ustawia awaryjną teksturę używaną przez encję. */
		Derived& setTexture(const std::shared_ptr<Texture>& texture) {
			_entity->_texture = texture;
			return self();
		}

		/** @brief Ustawia bazową prędkość ruchu. */
		Derived& setMovementSpeed(float speed) {
			_entity->_movement_speed = speed;
			return self();
		}

		/** @brief Ustawia rotację encji w stopniach. */
		Derived& setRotation(float rotation) {
			_entity->_rotation = rotation;
			return self();
		}

		/** @brief Podpina manager audio do encji. */
		Derived& setAudioManager(Audio::AudioManager* audio_manager) {
			_entity->_audio_manager = audio_manager;
			return self();
		}

		/** @brief Ustawia pozycję logiczną encji. */
		Derived& setPosition(Vector2 pos) {
			_entity->_pos = pos;
			return self();
		}

		/** @brief Ustawia współrzędną X encji. */
		Derived& setX(float x) {
			_entity->_pos.x = x;
			return self();
		}

		/** @brief Ustawia współrzędną Y encji. */
		Derived& setY(float y) {
			_entity->_pos.y = y;
			return self();
		}

		/** @brief Ustawia maksymalne HP i wyrównuje do niego obecne HP. */
		Derived& setMaxHp(int max_hp) {
			_entity->_max_hp = max_hp;
			_entity->_hp = max_hp;
			return self();
		}

	protected:
		Entity* _entity = nullptr;

		Derived& self() {
			return static_cast<Derived&>(*this);
		}
	};

} // namespace Nawia::Entity
