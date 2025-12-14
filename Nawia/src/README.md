# Core - Game

Map shall be created using **Tiled**.

## Tiles

All tiles should be created in .tsx folder in **/tiled/tiles.tsx**. Textures should also be uploaded to **/assets/textures/**.

Add a new tile:
* Open the .tsx file
* Click "plus" button
* Select 128x64 texture
* Add new un-standard properties, such as "isWalkable".
* Export file

Add new properties:
* Add a new property in Tiled and export .tsx
* Add a new property in **Tile.h** and create a setter
* In Map.cpp, search for "PROPERTIES" and follow instructions

Loading a map:
* Create a map in Tiled (orientation: iso; format: csv; order: right-bottom; size: infinite; width: 128; height: 64), save it as .tmx in /tiled/maps and export it as .json in /assets/maps
* Use Map::loadMap function and pass a file, ex. loadMap("map1.json").
* **REMEMBER TO USE THE SAME TILES (.TSX) FILE FOR ALL MAPS**