cmake_minimum_required(VERSION 3.25)

project(star_judgement)

add_compile_definitions(VULKAN_HPP_NO_EXCEPTIONS)
add_compile_definitions(DEBUG)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)
add_subdirectory(third_party)

target_link_libraries(source PRIVATE third_party)
