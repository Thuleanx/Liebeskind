cmake_minimum_required(VERSION 3.20)

project(imgui)

set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../imgui/)

add_library(imgui)
target_sources(imgui
    PUBLIC
    ${ROOT}/imconfig.h
    ${ROOT}/imgui.h
    ${ROOT}/imgui.cpp
    ${ROOT}/imstb_rectpack.h
    ${ROOT}/imstb_textedit.h
    ${ROOT}/imstb_truetype.h
    ${ROOT}/imgui_internal.h
    ${ROOT}/backends/imgui_impl_vulkan.h
    ${ROOT}/backends/imgui_impl_sdl3.h
    PRIVATE
    ${ROOT}/imgui_demo.cpp
    ${ROOT}/imgui_draw.cpp
    ${ROOT}/imgui_tables.cpp
    ${ROOT}/imgui_widgets.cpp
    ${ROOT}/backends/imgui_impl_vulkan.cpp
    ${ROOT}/backends/imgui_impl_sdl3.cpp
)

target_include_directories(imgui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    ${ROOT}
    $<INSTALL_INTERFACE:include>
)
target_compile_options(imgui PUBLIC -O2 -w)
target_link_options(imgui PUBLIC)

include(FindVulkan)
if(NOT VULKAN_FOUND)
    message(FATAL_ERROR "Vulkan SDK not installed.")
endif()

target_link_libraries(imgui PUBLIC Vulkan::Headers SDL3::SDL3)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan)
