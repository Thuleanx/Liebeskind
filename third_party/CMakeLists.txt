add_library(third_party INTERFACE)

add_subdirectory(Vulkan-Headers)
add_subdirectory(SDL)
add_subdirectory(plog)

include(FindVulkan)
if(NOT VULKAN_FOUND)
    message(FATAL_ERROR "Vulkan SDK not installed.")
endif()

target_link_libraries(third_party INTERFACE Vulkan::Headers SDL3::SDL3 plog::plog)
target_link_libraries(third_party INTERFACE Vulkan::Vulkan)
# target_compile_definitions(third_party INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
