add_library(GameLib
    Game.cpp
    Entities/Player.cpp
    Entities/Enemy.cpp
    World/Map.cpp
)

target_include_directories(GameLib PUBLIC 
    ${CMAKE_SOURCE_DIR}/src/Game
    ${CMAKE_SOURCE_DIR}/src/Game/Entities
    ${CMAKE_SOURCE_DIR}/src/Game/World
)
target_link_libraries(GameLib PRIVATE CoreLib SDL3::SDL3)
