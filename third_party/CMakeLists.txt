add_library(third_party INTERFACE)

add_subdirectory(Vulkan-Headers)
add_subdirectory(SDL)

target_link_libraries(third_party INTERFACE Vulkan::Headers SDL3::SDL3)
