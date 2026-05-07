# Audio Guide

Ten przewodnik opisuje uzycie `Nawia::Audio::AudioManager`.

## Format assetow

- Muzyka: preferuj `.ogg` dla dobrej petli i rozmiaru; `.mp3` jest OK dla dluzszych utworow, ale gorzej nadaje sie do idealnych loopow.
- Efekty: preferuj `.wav` dla bardzo krotkich, czesto odpalanych dzwiekow; `.ogg` jest OK, gdy rozmiar pliku ma znaczenie.
- Sciezki zapisuj wzgledem runtime gry, np. `assets/audio/sfx/sword_slash.wav`.

Proponowana struktura:

```text
assets/audio/
    music/
    sfx/
        player/
        enemies/
        ui/
        ambience/
```

## Efekty dzwiekowe

Efekty sa ladowane do cache przez stabilne ID. Gameplay powinien odpalac ID, a nie bezposrednio operowac na `Sound`.

```cpp
auto& audio = engine->getAudioManager();
audio.loadSound("player.sword_slash", "assets/audio/sfx/player/sword_slash.wav");
audio.playSound("player.sword_slash");
```

Mozna tez zaladowac i odtworzyc jednym wywolaniem:

```cpp
audio.playSoundFile("enemy.walking_dead.roar", "assets/audio/sfx/enemies/walking_dead_roar.ogg");
```

`SoundOptions` pozwala ustawic lokalna glosnosc, pitch i zachowanie przy ponownym odpaleniu tego samego efektu.

## Muzyka

Pojedynczy utwor:

```cpp
audio.playMusic("assets/audio/music/main_theme.ogg", true);
```

Playlista sekwencyjna:

```cpp
audio.playPlaylist({
    "assets/audio/music/forest_01.ogg",
    "assets/audio/music/forest_02.ogg"
});
```

Playlista losowa bez powtorzen do konca cyklu:

```cpp
Nawia::Audio::PlaylistOptions options;
options.mode = Nawia::Audio::PlaylistMode::Random;
options.loop = true;
audio.playPlaylist(tracks, options);
```

## Glosnosc

`Settings` przechowuje trzy niezalezne wartosci:

- `master_volume`
- `music_volume`
- `effects_volume`

`Engine::applySettings()` przekazuje je do `AudioManager`, a menu ustawien ma osobna zakladke `DZWIEK`.
