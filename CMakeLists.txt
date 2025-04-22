cmake_minimum_required(VERSION 3.25)

project(star_judgement)

add_compile_definitions(VULKAN_HPP_NO_EXCEPTIONS)
add_compile_definitions(DEBUG)
add_compile_definitions(GLM_FORCE_RADIANS)
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

# glm uses depth -1 to 1 by default, which is in line with
# openGL. Since we are using Vulkan we want it to be 0-1
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(src)
add_subdirectory(third_party)

target_link_libraries(source PRIVATE third_party)

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
