# Untitled

W `Entity::loadModel()` każda encja robi:

```cpp
_model = LoadModel(path.c_str());
...
addAnimation("default", path);
```

a destruktor każdej encji robi potem `UnloadModel(_model)` i `UnloadModelAnimation(...)`. Czyli każdy spawn tego samego typu przeciwnika/obiektu ponownie czyta plik, parsuje `.glb`, ładuje animacje i uploaduje meshe do GPU. To dokładnie pasuje do spadków FPS przy pojawieniu się obiektu. Widać to w `Entity.cpp`: `LoadModel()` jest wywoływany per encja, a `UnloadModel()` per encja w destruktorze. ([GitHub](https://github.com/m-sadkowski/Nawia_ARPG/blob/dev/Nawia/src/entity/Entity.cpp))

## 1. Czy `LoadModel()` można przyspieszyć wielowątkowo?

**Nie bezpośrednio.** `LoadModel()` w raylib na końcu robi `UploadMesh(&model.meshes[i], false)`, czyli tworzy VAO/VBO i wysyła dane do GPU. To musi iść przez aktywny kontekst OpenGL, zwykle główny/renderujący wątek. ([GitHub](https://raw.githubusercontent.com/raysan5/raylib/master/src/rmodels.c)) OpenGL nie pozwala używać tego samego kontekstu równolegle z wielu wątków; kontekst może być aktualny tylko w jednym wątku naraz. ([Khronos Forums](https://community.khronos.org/t/is-opengl-thread-safe/37379))

Możesz natomiast zrobić **asynchroniczne przygotowanie**, ale rozdzielone:

```
worker thread:
  - czyta JSON/mapę/listę assetów
  - przygotowuje listę modeli do załadowania
  - ewentualnie ładuje surowe dane do RAM własnym loaderem

main/render thread:
  - wykonuje LoadModel / UploadMesh / LoadTextureFromImage
  - robi to na loading screenie albo porcjami, np. 1 asset na klatkę
```

Dla tekstur raylib ma naturalny podział: `Image` jest w RAM, `Texture` jest w VRAM; teksturę do VRAM trzeba tworzyć na wątku z kontekstem OpenGL. Analogiczny problem pokazuje się przy `LoadTexture()` w osobnym wątku. ([Stack Overflow](https://stackoverflow.com/questions/79287456/using-threading-to-load-textures-in-raylib-is-failing))

## 2. Czy możemy użyć VRAM/RAM? Tak — ale `LoadModel()` już używa VRAM

Raylib rozróżnia dane CPU i GPU: `Image` to surowe piksele w RAM, `Texture` ma OpenGL texture id i jest w GPU/VRAM; `Mesh` zawiera tablice wierzchołków oraz identyfikatory VAO/VBO, a `Model` trzyma meshe i materiały. ([GitHub](https://raw.githubusercontent.com/raysan5/raylib/master/src/raylib.h)) Raylib API mówi też wprost, że `UploadMesh()` uploaduje vertex data do GPU i nadaje VAO/VBO, a `UnloadMesh()` zwalnia mesh z CPU i GPU. ([GitHub](https://github.com/raysan5/raylib.com/blob/master/cheatsheet/raylib_models.c))

Problem u Ciebie jest taki, że **każda instancja obiektu ładuje własną kopię modelu**, więc duplikujesz:

```
ten sam plik .glb
→ parsowanie na CPU
→ dane modelu w RAM
→ VBO/VAO w VRAM
→ animacje w RAM
```

Zamiast tego potrzebujesz cache’u modeli, analogicznie do istniejącego `ResourceManager::getTexture()`, który już cache’uje tekstury. ([GitHub](https://github.com/m-sadkowski/Nawia_ARPG/blob/dev/Nawia/src/core/util/ResourceManager.cpp))

## 3. Najważniejsza zmiana: cache `Model`, nie `LoadModel()` w każdej encji

Docelowo encja nie powinna posiadać `Model _model` jako owner. Powinna mieć wskaźnik do współdzielonego zasobu:

```cpp
// ResourceManager.h
class ResourceManager {
public:
    std::shared_ptr<Model> getModel(const std::string& filename);

private:
    std::map<std::string, std::shared_ptr<Model>> _models;
};
```

```cpp
// ResourceManager.cpp
std::shared_ptr<Model> ResourceManager::getModel(const std::string& filename) {
    auto it = _models.find(filename);
    if (it != _models.end()) {
        return it->second;
    }

    Model model = LoadModel(filename.c_str());

    if (model.meshCount == 0) {
        Logger::errorLog("ResourceManager: nie udalo sie zaladowac modelu: " + filename);
        return nullptr;
    }

    auto loadedModel = std::shared_ptr<Model>(
        new Model(model),
        [](Model* modelToUnload) {
            UnloadModel(*modelToUnload);
            delete modelToUnload;
        }
    );

    _models[filename] = loadedModel;
    return loadedModel;
}
```

W `Entity` zamiast:

```cpp
Model _model = {};
bool _model_loaded = false;
```

daj:

```cpp
std::shared_ptr<Model> _model;
```

i render:

```cpp
if (_model) {
    DrawModelEx(*_model, pos3d, {0.0f, 1.0f, 0.0f}, visual_rotation, {_scale, _scale, _scale}, WHITE);
}
```

Destruktor `Entity` **nie powinien już robić `UnloadModel()`**, bo model należy do `ResourceManagera`. Dzięki temu 50 bandytów używa jednego modelu w VRAM zamiast 50 kopii.

## 4. Animacje też cache’ować, ale ostrożnie

U Ciebie `loadModel()` automatycznie robi `addAnimation("default", path)`, a `addAnimation()` wywołuje `LoadModelAnimations()` za każdym razem. ([GitHub](https://github.com/m-sadkowski/Nawia_ARPG/blob/dev/Nawia/src/entity/Entity.cpp)) W `Player` w konstruktorze ładowanych jest kilka osobnych plików animacji `.glb`: idle, walk, attack, knocked, stand_up. ([GitHub](https://github.com/m-sadkowski/Nawia_ARPG/blob/dev/Nawia/src/entity/actors/player/Player.cpp)) Jeśli podobnie robisz dla przeciwników przy spawnie, to każdy spawn ponownie ładuje animacje z dysku.

Zrób osobny cache:

```cpp
struct AnimationSet {
    std::vector<ModelAnimation> animations;

    ~AnimationSet() {
        for (auto& anim : animations) {
            UnloadModelAnimation(anim);
        }
    }
};
```

Encja powinna trzymać tylko:

```cpp
std::shared_ptr<AnimationSet> _animationSet;
int _current_anim_index;
float _anim_frame_counter;
```

Uwaga: `UpdateModelAnimation()` aktualizuje pose modelu, vertex buffery i bone matrices. ([GitHub](https://github.com/raysan5/raylib.com/blob/master/cheatsheet/raylib_models.c)) Dlatego **dla animowanych encji nie kopiuj ślepo jednego `Model` między wieloma niezależnymi postaciami**, jeśli każda ma inną klatkę animacji. Dla statycznych modeli cache jest prosty. Dla animowanych masz 3 dobre opcje:

1. **Cache plików i animacji, ale twórz/pooluj instancje modeli z wyprzedzeniem** — najlepsze na teraz.
2. **Object pool dla przeciwników** — tworzysz np. 20 bandytów podczas ładowania levelu, a potem tylko aktywujesz/dezaktywujesz.
3. **Własny renderer animacji/GPU skinning/per-instance bone matrices** — najlepsze długoterminowo, ale dużo większy refactor.

## 5. Najprostszy plan naprawy dla Nawia_ARPG

Najpierw zrobiłbym to:

```
1. Dodać ResourceManager::getModel()
2. Przenieść ownership Model z Entity do ResourceManagera
3. Usunąć UnloadModel z Entity::~Entity()
4. Dodać preload modeli przy ładowaniu levelu
5. Dodać cache animacji albo przynajmniej preload animacji
6. Dla przeciwników zrobić object pool
```

Czyli spawn przeciwnika nie powinien robić:

```cpp
new Bandit()
  -> LoadModel("assets/models/bandit.glb")
  -> LoadModelAnimations(...)
```

tylko:

```cpp
new Bandit()
  -> _model = resourceManager.getModel("assets/models/bandit.glb")
  -> _animations = resourceManager.getAnimations("bandit")
```

albo jeszcze lepiej:

```
Level loading:
  preload bandit model
  preload bandit animations
  create pool of 20 Bandit instances

Gameplay:
  activate Bandit from pool
```

## 6. Dla wielu takich samych statycznych obiektów: instancing

Jeżeli masz dużo identycznych modeli typu kamienie, drzewa, beczki, trawy, dekoracje, nie renderuj ich jako setek `DrawModelEx()`. Raylib ma `DrawMeshInstanced()`, które rysuje wiele instancji tego samego mesha z różnymi transformacjami. ([GitHub](https://github.com/raysan5/raylib.com/blob/master/cheatsheet/raylib_models.c))

To nie zastępuje `Model` 1:1, bo `Model` może mieć wiele meshów i materiałów, ale dla prostych statycznych propów możesz zrobić pętlę po meshach:

```cpp
for (int i = 0; i < model.meshCount; ++i) {
    int matId = model.meshMaterial[i];
    DrawMeshInstanced(
        model.meshes[i],
        model.materials[matId],
        transforms.data(),
        static_cast<int>(transforms.size())
    );
}
```

To ograniczy draw calle i CPU overhead renderowania wielu identycznych obiektów.

## 7. Drobne rzeczy, które też u Ciebie kosztują

W `Entity::render()` hover robi drugi `DrawModelEx()`, czyli obiekt pod kursorem renderuje się drugi raz. Debug collidery też są opisane jako drogie i renderują bbox/mesh collision debug. ([GitHub](https://github.com/m-sadkowski/Nawia_ARPG/raw/refs/heads/dev/Nawia/src/entity/Entity.cpp)) To raczej nie jest główny problem przy spawnie, ale warto pamiętać.

Największy win będzie z tego:

```
Nie ładuj modelu przy spawnie.
Ładuj model raz.
Trzymaj go w ResourceManagerze.
Encje mają tylko referencję i transform.
Animowane encje pooluj albo prewarmuj.
```

W Twoim przypadku zacząłbym od cache’u modeli i preloadu levelu — to powinno usunąć największy drop FPS przy pojawianiu się obiektów.