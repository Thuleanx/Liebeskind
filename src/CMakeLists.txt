add_executable(source main.cpp)
target_compile_features(source PRIVATE cxx_std_23)
set_target_properties(source PROPERTIES CXX_EXTENSIONS off CXX_STD_REQUIRED on)

if (MSVC)
    # Warning all, sdl = security, extern = ignore external headers warnings for angle brackets includes
    target_compile_options(source PRIVATE /W4 /Wall /sdl /extern:anglebrackets /extern:W2 /fsanitize=address)
else ()
    target_compile_options(source PRIVATE -Wall -Wextra -Wpedantic)
endif ()
