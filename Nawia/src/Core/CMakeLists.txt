add_library(CoreLib
    Camera.cpp
    Utils.h
)

target_include_directories(CoreLib PUBLIC ${CMAKE_SOURCE_DIR}/src/Core)
target_link_libraries(CoreLib PRIVATE SDL3::SDL3)
