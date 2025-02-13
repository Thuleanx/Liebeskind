add_library(third_party INTERFACE)

set(SDL_STATIC ON CACHE INTERNAL "")
add_subdirectory(Vulkan-Headers)
add_subdirectory(SDL)
add_subdirectory(plog)
add_subdirectory(tinyobjloader)
add_subdirectory(imgui)

add_library(stb_image)
target_sources(stb_image PRIVATE "stb/src/stb_image.cpp")
target_include_directories(stb_image PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/stb")

include(FindVulkan)
if(NOT VULKAN_FOUND)
    message(FATAL_ERROR "Vulkan SDK not installed.")
endif()
target_link_libraries(third_party INTERFACE Vulkan::Headers SDL3::SDL3 plog::plog)
target_link_libraries(third_party INTERFACE Vulkan::Vulkan)
target_link_libraries(third_party INTERFACE stb_image)
target_link_libraries(third_party INTERFACE imgui)
target_link_libraries(third_party INTERFACE tinyobjloader)
# target_compile_definitions(third_party INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

