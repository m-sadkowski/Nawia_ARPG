add_subdirectory(Core)
add_subdirectory(Game)

add_executable(Nawia main.cpp)

target_link_libraries(Nawia PRIVATE GameLib CoreLib SDL3::SDL3)

add_custom_command(
    TARGET Nawia POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_BINARY_DIR}/$<CONFIG>/SDL3.dll"
        "$<TARGET_FILE_DIR:Nawia>"
    COMMENT "Kopiowanie SDL3.dll do katalogu wykonawczego"
)
