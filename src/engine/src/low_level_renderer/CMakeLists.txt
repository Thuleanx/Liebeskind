set(SRC 
    graphics_device_interface.cpp 
    descriptor_allocator.cpp 
    mesh.cpp
    swapchain_data.cpp
    vertex_buffer.cpp
    uniform_buffer.cpp
    private/sampler.cpp
    private/command.cpp
    private/command.h
    private/image.cpp
    private/image.h
    private/texture.cpp
    private/graphics_device_helper.h
    private/graphics_device_helper.cpp
    private/queue_family.h
    private/queue_family.cpp
    private/validation.h
    private/validation.cpp
    private/swapchain.h
    private/swapchain.cpp
    private/buffer.h
    private/buffer.cpp
)

add_library(low_level_renderer ${SRC})

target_link_libraries(low_level_renderer PRIVATE third_party)
target_link_libraries(low_level_renderer PRIVATE logger)
target_link_libraries(low_level_renderer PRIVATE file_system)

