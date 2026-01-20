cmake_minimum_required(VERSION 3.20)
project(Nawia)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files (globbing to catch all cpp files in src recursively)
# Triggering re-scan for new files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Include directories
include_directories(
    src/audio
    src/core
    src/core/game
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
    src/entity/actors/enemies/dummy
    src/entity/actors/enemies/walking_dead
    src/entity/components
    src/entity/systems
    src/entity/effects
    src/entity/abilities
    src/entity/abilities/projectiles
    src/entity/abilities/melee
    src/entity/abilities/projectiles/Fireball
    src/entity/abilities/melee/SwordSlash
    src/entity/interactive
    src/entity/interactive/onTrigger
    src/entity/interactive/clickable
    src/entity/interactive/clickable/chest
    src/world
    src/world/map
    src/world/level
    src/ui
    src/ui/hud
    src/ui/inventory
    src/ui/menu
    src/item
    src/item/inventory
    src/item/types
    external/json
)


include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
)
FetchContent_MakeAvailable(raylib)

add_executable(Nawia ${SOURCES})

# Link libraries
target_link_libraries(Nawia PRIVATE raylib)

# Copy assets to build directory
add_custom_command(TARGET Nawia POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    ${CMAKE_SOURCE_DIR}/out/build/assets
)