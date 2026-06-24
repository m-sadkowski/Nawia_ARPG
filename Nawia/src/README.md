# Struktura `src`

Ten katalog zawiera kod gry Nawia.

## Glowne moduly

- `audio/` - manager efektow dzwiekowych, muzyki, playlist i glosnosci.
- `core/` - petla silnika, kamera, mapa, input, resource manager, questy i systemy pomocnicze.
- `entity/` - encje swiata, aktorzy, ability, collidery i interakcje.
- `item/` - itemy, backpack, equipment, baza itemow i loottable.
- `ui/` - HUD, inventory, chest, questbook, dialogi i menu.
- `world/` - levele, spawn manager, factory i navmesh.

## Runtime i assety

Projekt docelowo uruchamiamy z:

```text
Nawia/out/build/x64-Release
```

Sciezki w danych powinny byc dopasowane do tego runtime, np.:

```text
assets/textures/items/key.png
assets/data/abilities.json
assets/maps/demo_map/demo.glb
```

## Najwazniejsze przewodniki

- `docs/guides/Coding_Standards.md`
- `docs/guides/Entity_Guide.md`
- `docs/guides/Enemy_Guide.md`
- `docs/guides/Ability_Guide.md`
- `docs/guides/AbilityEffect_Guide.md`
- `docs/guides/Audio_Guide.md`
- `docs/guides/Level_Guide.md`
- `docs/guides/Interactive_Guide.md`
- `docs/guides/Item_Guide.md`
- `docs/guides/UI_Guide.md`
