cmake_minimum_required(VERSION 3.25)

project(star_judgement)

add_compile_definitions(VULKAN_HPP_NO_EXCEPTIONS)
add_compile_definitions(DEBUG)
add_compile_definitions(GLM_FORCE_RADIANS)

# glm uses depth -1 to 1 by default, which is in line with
# openGL. Since we are using Vulkan we want it to be 0-1
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(src)
add_subdirectory(third_party)

target_link_libraries(source PRIVATE third_party)
