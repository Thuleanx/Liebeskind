add_library(third_party INTERFACE)

set(SDL_TEST OFF)
set(SDL_STATIC ON CACHE INTERNAL "")
add_subdirectory(Vulkan-Headers EXCLUDE_FROM_ALL)
add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_subdirectory(plog EXCLUDE_FROM_ALL)
add_subdirectory(json EXCLUDE_FROM_ALL)
add_subdirectory(tinyobjloader EXCLUDE_FROM_ALL)
add_subdirectory(imgui_cmake EXCLUDE_FROM_ALL)

# include(FetchContent)
# FetchContent_Declare(
#   cpptrace
#   GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
#   GIT_TAG        v1.0.4 # <HASH or TAG>
# )
# FetchContent_MakeAvailable(cpptrace)
# target_link_libraries(third_party INTERFACE cpptrace::cpptrace)

add_library(stb_image)
target_sources(stb_image PRIVATE "stb_image.cpp")
target_include_directories(stb_image PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/stb")

set(ENABLE_OPT 0)
set(CMAKE_BUILD_TYPE "Release")
add_subdirectory(glslang EXCLUDE_FROM_ALL)

include(FindVulkan)
if(NOT VULKAN_FOUND)
    message(FATAL_ERROR "Vulkan SDK not installed.")
endif()

# target_link_libraries(third_party INTERFACE cpptrace::cpptrace)
target_link_libraries(third_party INTERFACE Vulkan::Headers SDL3::SDL3 plog::plog)
target_link_libraries(third_party INTERFACE Vulkan::Vulkan)
target_link_libraries(third_party INTERFACE stb_image)
target_link_libraries(third_party INTERFACE imgui)
target_link_libraries(third_party INTERFACE nlohmann_json)
target_link_libraries(third_party INTERFACE tinyobjloader)
target_link_libraries(third_party INTERFACE glslang SPIRV glslang-default-resource-limits)
