add_library(file_system file.cpp)

add_compile_definitions(VULKAN_HPP_NO_EXCEPTIONS)

target_link_libraries(file_system PRIVATE third_party)

target_link_libraries(file_system PRIVATE logger)
