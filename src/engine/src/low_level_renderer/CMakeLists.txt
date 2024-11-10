set(SRC 
    graphics_device_interface.cpp 
    private/graphics_device_helper.h
    private/graphics_device_helper.cpp
    private/queue_family.h
    private/queue_family.cpp
    private/validation.h
    private/validation.cpp
    private/swapchain.h
    private/swapchain.cpp
)

add_library(low_level_renderer ${SRC})

target_compile_features(low_level_renderer PUBLIC cxx_std_17)

target_link_libraries(low_level_renderer PRIVATE third_party)
target_link_libraries(low_level_renderer PRIVATE logger)
target_link_libraries(low_level_renderer PRIVATE file_system)

