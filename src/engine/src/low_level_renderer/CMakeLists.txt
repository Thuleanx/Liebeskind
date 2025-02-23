set(SRC 
    _impl_drawing.cpp
    graphics_module.cpp
    render_submission.cpp
    material_pipeline.cpp
    graphics_device_interface.cpp 
    graphics_user_interface.cpp 
    descriptor_write_buffer.cpp 
    descriptor_allocator.cpp 
    swapchain_data.cpp
    vertex_buffer.cpp
    uniform_buffer.cpp
    queue_family.cpp
    private/sampler.cpp
    private/command.cpp
    private/command.h
    private/image.cpp
    private/image.h
    private/texture.cpp
    private/graphics_device_helper.h
    private/graphics_device_helper.cpp
    private/validation.h
    private/validation.cpp
    private/swapchain.h
    private/swapchain.cpp
    private/buffer.h
    private/buffer.cpp
)

add_library(low_level_renderer ${SRC})

target_link_libraries(low_level_renderer PRIVATE resource_management)
target_link_libraries(low_level_renderer PRIVATE third_party)
target_link_libraries(low_level_renderer PRIVATE logger)
target_link_libraries(low_level_renderer PRIVATE file_system)

