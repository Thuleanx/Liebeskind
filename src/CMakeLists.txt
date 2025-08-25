add_executable(source main.cpp)

set_target_properties(source PROPERTIES CXX_EXTENSIONS off CXX_STD_REQUIRED on)

if (MSVC)
    # Warning all, sdl = security, extern = ignore external headers warnings for angle brackets includes
    target_compile_options(source PRIVATE /W4 /Wall /sdl /extern:anglebrackets /extern:W2 /fsanitize=address)
else ()
    target_compile_options(source PRIVATE -Wall -Wextra -Wpedantic -Werror -Wfloat-equal -pedantic-errors -Wold-style-cast -DNDEBUG -g -ggdb)
endif ()

add_subdirectory(engine)
add_subdirectory(game)

target_link_libraries(source PUBLIC engine)
target_link_libraries(source PUBLIC game)

# Needed for shared library builds on windows:  copy cpptrace.dll to the same directory as the
# executable for your_target
if(WIN32)
  add_custom_command(
    TARGET source POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:cpptrace::cpptrace>
    $<TARGET_FILE_DIR:source>
  )
endif()
