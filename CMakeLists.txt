cmake_minimum_required(VERSION 3.25)
project(star_judgement)

add_subdirectory(src)
add_subdirectory(third_party)

target_link_libraries(source PRIVATE third_party)
