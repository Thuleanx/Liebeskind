add_library(third_party INTERFACE)

add_subdirectory(Vulkan-Headers)
add_subdirectory(SDL)

include(FindVulkan)
if(NOT VULKAN_FOUND)
    message(FATAL_ERROR "Vulkan SDK not installed.")
endif()

target_link_libraries(third_party INTERFACE Vulkan::Headers SDL3::SDL3)
target_link_libraries(third_party INTERFACE Vulkan::Vulkan)
