cmake_minimum_required(VERSION 3.25)

project(star_judgement)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# C files should now compile as C++
# list(APPEND CMAKE_CXX_SOURCE_FILE_EXTENSIONS c)

set_property(GLOBAL PROPERTY CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Force static linking for libgcc and libstdc++
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
 
# Static link libwinpthread (if using threads)
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic")

# Make it so that CMake outputs dlls into the binary directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_compile_definitions(VULKAN_HPP_NO_EXCEPTIONS)
add_compile_definitions(VULKAN_HPP_ASSERT_ON_RESULT=VULKAN_HPP_NAMESPACE::detail::ignore)
add_compile_definitions(DEBUG)
add_compile_definitions(GLM_FORCE_RADIANS)
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

# glm uses depth -1 to 1 by default, which is in line with
# openGL. Since we are using Vulkan we want it to be 0-1
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

# set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

include(NoRttiOrExceptions.cmake)

find_package(Threads REQUIRED)

add_subdirectory(src)
add_subdirectory(third_party EXCLUDE_FROM_ALL)

target_link_libraries(source PUBLIC third_party)

add_custom_target(Scenes)

# Copy over files under scenes
add_custom_command(
    TARGET Scenes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/scenes/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory              
    "${CMAKE_CURRENT_SOURCE_DIR}/scenes" "${PROJECT_BINARY_DIR}/scenes"
    COMMENT "Copying scenes" VERBATIM
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:source>/scenes/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_BINARY_DIR}/scenes"
    "$<TARGET_FILE_DIR:source>/scenes"
)

add_dependencies(source Scenes)

add_custom_target(Bundled)

# Copy over dlls
add_custom_command(
    TARGET Bundled POST_BUILD
    COMMENT "Copying dlls" VERBATIM
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/bundled"
    "$<TARGET_FILE_DIR:source>"
)

add_dependencies(source Bundled)
