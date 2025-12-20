cmake_minimum_required(VERSION 3.20)
project(Nawia)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files (globbing to catch all cpp files in src recursively)
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
    src/entity/actors
    src/entity/components
    src/entity/systems
    src/entity/effects
    src/entity/abilities
    src/entity/abilities/Fireball
    src/entity/abilities/SwordSlash
    src/world
    src/world/map
    src/world/level
    src/ui
    src/ui/hud
    src/ui/menu
    external/SDL3/include
    external/SDL3_image/include
    external/SDL3_mixer/include
    external/json
)

# Library directories
link_directories(
    external/SDL3/lib/x64
    external/SDL3_image/lib/x64
    external/SDL3_mixer/lib
)

add_executable(Nawia ${SOURCES})

# Link libraries
target_link_libraries(Nawia PRIVATE
    SDL3
    SDL3_image
    SDL3_mixer
)

# Post-build copy dlls
add_custom_command(TARGET Nawia POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/SDL3/lib/x64/SDL3.dll"
        $<TARGET_FILE_DIR:Nawia>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/SDL3_image/lib/x64/SDL3_image.dll"
        $<TARGET_FILE_DIR:Nawia>
    # SDL3_mixer has multiple DLLs in bin/ which might be needed
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/external/SDL3_mixer/bin"
        $<TARGET_FILE_DIR:Nawia>
)