add_executable(source main.cpp)

target_compile_features(source PRIVATE cxx_std_23)
set_target_properties(source PROPERTIES CXX_EXTENSIONS off CXX_STD_REQUIRED on)
add_compile_definitions(DEBUG)

target_compile_features(source PUBLIC cxx_std_17)

if (MSVC)
    # Warning all, sdl = security, extern = ignore external headers warnings for angle brackets includes
    target_compile_options(source PRIVATE /W4 /Wall /sdl /extern:anglebrackets /extern:W2 /fsanitize=address)
else ()
    target_compile_options(source PRIVATE -Wall -Wextra -Wpedantic)
endif ()

add_subdirectory(engine)
add_subdirectory(game)

target_link_libraries(source PUBLIC engine)
add_custom_command(TARGET source POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:source>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:source>/shaders"
        )
