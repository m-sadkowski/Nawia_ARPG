cmake_minimum_required(VERSION 3.20)
project(Nawia)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files (globbing to catch all cpp files in src recursively)
# Triggering re-scan for new files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# Include directories
include_directories(
    src/audio
    src/core
    src/core/game
    src/core/game/agent
    src/core/game/combat
    src/core/game/ping
    src/core/game/telemetry
    src/core/game/save
    src/core/util
    src/core/system
    src/core/system/input
    src/core/system/time
    src/core/system/renderer
    src/entity
    src/entity/collider
    src/entity/actors
    src/entity/actors/enemies
    src/entity/actors/player
    src/entity/actors/enemies/walking_dead
    src/entity/actors/enemies/devil
    src/entity/actors/enemies/bandit
    src/entity/actors/enemies/rift_binder
    src/entity/actors/enemies/spider
    src/entity/actors/enemies/witch
    src/entity/actors/allies
    src/entity/actors/allies/friend
    src/entity/components
    src/entity/systems
    src/entity/effects
    src/entity/abilities
    src/entity/abilities/projectiles
    src/entity/abilities/melee
    src/entity/abilities/projectiles/Fireball
    src/entity/abilities/projectiles/KnifeThrow
    src/entity/abilities/melee/SwordSlash
    src/entity/interactive
    src/entity/interactive/onTrigger
    src/entity/interactive/clickable
    src/entity/interactive/clickable/chest
    src/entity/interactive/clickable/NPC
    src/world
    src/world/map
    src/world/level
    src/world/spawn
    src/ui
    src/ui/hud
    src/ui/inventory
    src/ui/menu
    src/ui/dialogue
    src/ui/quest
    src/core/game/quest
    src/core/game/boss
    src/item
    src/item/inventory
    src/item/types
    src/item/utils
    external/json
    ${recastnavigation_SOURCE_DIR}/Recast/Include
    ${recastnavigation_SOURCE_DIR}/Detour/Include
)

include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

# TinyXML2 for parsing .tsx tileset files
FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG 10.0.0
)
FetchContent_MakeAvailable(tinyxml2)

# Recast Navigation
FetchContent_Declare(
    recastnavigation
    GIT_REPOSITORY https://github.com/recastnavigation/recastnavigation.git
    GIT_TAG main
)
# Recast builds tests by default, disable them to speed up build
set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(recastnavigation)

add_executable(Nawia ${SOURCES})

# Link libraries
target_link_libraries(Nawia PRIVATE raylib tinyxml2 Recast Detour)

if(WIN32)
    target_link_libraries(Nawia PRIVATE ws2_32)
endif()

# Copy assets next to Nawia.exe.
add_custom_command(TARGET Nawia POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:Nawia>/assets"
    VERBATIM
)
